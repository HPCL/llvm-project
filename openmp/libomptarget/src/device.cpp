//===--------- device.cpp - Target independent OpenMP target RTL ----------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// Functionality for managing devices that are handled by RTL plugins.
//
//===----------------------------------------------------------------------===//

#include "device.h"
#include "private.h"
#include "rtl.h"

#include <cassert>
#include <climits>
#include <string>

/// Map between Device ID (i.e. openmp device id) and its DeviceTy.
DevicesTy Devices;

int DeviceTy::associatePtr(void *HstPtrBegin, void *TgtPtrBegin, int64_t Size) {
  DataMapMtx.lock();

  // Check if entry exists
  for (auto &HT : HostDataToTargetMap) {
    if ((uintptr_t)HstPtrBegin == HT.HstPtrBegin) {
      // Mapping already exists
      bool isValid = HT.HstPtrBegin == (uintptr_t) HstPtrBegin &&
                     HT.HstPtrEnd == (uintptr_t) HstPtrBegin + Size &&
                     HT.TgtPtrBegin == (uintptr_t) TgtPtrBegin;
      DataMapMtx.unlock();
      if (isValid) {
        DP("Attempt to re-associate the same device ptr+offset with the same "
            "host ptr, nothing to do\n");
        return OFFLOAD_SUCCESS;
      } else {
        DP("Not allowed to re-associate a different device ptr+offset with the "
            "same host ptr\n");
        return OFFLOAD_FAIL;
      }
    }
  }

  // Mapping does not exist, allocate it with refCount=INF
  HostDataToTargetTy newEntry((uintptr_t) HstPtrBegin /*HstPtrBase*/,
                              (uintptr_t) HstPtrBegin /*HstPtrBegin*/,
                              (uintptr_t) HstPtrBegin + Size /*HstPtrEnd*/,
                              (uintptr_t) TgtPtrBegin /*TgtPtrBegin*/,
                              true /*IsRefCountINF*/);

  DP("Creating new map entry: HstBase=" DPxMOD ", HstBegin=" DPxMOD ", HstEnd="
      DPxMOD ", TgtBegin=" DPxMOD "\n", DPxPTR(newEntry.HstPtrBase),
      DPxPTR(newEntry.HstPtrBegin), DPxPTR(newEntry.HstPtrEnd),
      DPxPTR(newEntry.TgtPtrBegin));
  HostDataToTargetMap.push_front(newEntry);

  DataMapMtx.unlock();

  return OFFLOAD_SUCCESS;
}

int DeviceTy::disassociatePtr(void *HstPtrBegin) {
  DataMapMtx.lock();

  // Check if entry exists
  for (HostDataToTargetListTy::iterator ii = HostDataToTargetMap.begin();
      ii != HostDataToTargetMap.end(); ++ii) {
    if ((uintptr_t)HstPtrBegin == ii->HstPtrBegin) {
      // Mapping exists
      if (ii->isRefCountInf()) {
        DP("Association found, removing it\n");
        HostDataToTargetMap.erase(ii);
        DataMapMtx.unlock();
        return OFFLOAD_SUCCESS;
      } else {
        DP("Trying to disassociate a pointer which was not mapped via "
            "omp_target_associate_ptr\n");
        break;
      }
    }
  }

  // Mapping not found
  DataMapMtx.unlock();
  DP("Association not found\n");
  return OFFLOAD_FAIL;
}

// Get ref count of map entry containing HstPtrBegin
uint64_t DeviceTy::getMapEntryRefCnt(void *HstPtrBegin) {
  uintptr_t hp = (uintptr_t)HstPtrBegin;
  uint64_t RefCnt = 0;

  DataMapMtx.lock();
  for (auto &HT : HostDataToTargetMap) {
    if (hp >= HT.HstPtrBegin && hp < HT.HstPtrEnd) {
      DP("DeviceTy::getMapEntry: requested entry found\n");
      RefCnt = HT.getRefCount();
      break;
    }
  }
  DataMapMtx.unlock();

  if (RefCnt == 0) {
    DP("DeviceTy::getMapEntry: requested entry not found\n");
  }

  return RefCnt;
}

LookupResult DeviceTy::lookupMapping(void *HstPtrBegin, int64_t Size) {
  uintptr_t hp = (uintptr_t)HstPtrBegin;
  LookupResult lr;

  DP("Looking up mapping(HstPtrBegin=" DPxMOD ", Size=%ld)...\n", DPxPTR(hp),
      Size);
  for (lr.Entry = HostDataToTargetMap.begin();
      lr.Entry != HostDataToTargetMap.end(); ++lr.Entry) {
    auto &HT = *lr.Entry;
    // Is it contained?
    lr.Flags.IsContained = hp >= HT.HstPtrBegin && hp < HT.HstPtrEnd &&
        (hp+Size) <= HT.HstPtrEnd;
    // Does it extend into an already mapped region?
    lr.Flags.ExtendsBefore = hp < HT.HstPtrBegin && (hp+Size) > HT.HstPtrBegin;
    // Does it extend beyond the mapped region?
    lr.Flags.ExtendsAfter = hp < HT.HstPtrEnd && (hp+Size) > HT.HstPtrEnd;

    if (lr.Flags.IsContained || lr.Flags.ExtendsBefore ||
        lr.Flags.ExtendsAfter) {
      break;
    }
  }

  if (lr.Flags.ExtendsBefore) {
    DP("WARNING: Pointer is not mapped but section extends into already "
        "mapped data\n");
  }
  if (lr.Flags.ExtendsAfter) {
    DP("WARNING: Pointer is already mapped but section extends beyond mapped "
        "region\n");
  }

  return lr;
}

// Used by target_data_begin
// Return the target pointer begin (where the data will be moved).
// Allocate memory if this is the first occurrence of this mapping.
// Increment the reference counter.
// If NULL is returned, then either data allocation failed or the user tried
// to do an illegal mapping.
void *DeviceTy::getOrAllocTgtPtr(void *HstPtrBegin, void *HstPtrBase,
    int64_t Size, bool &IsNew, bool &IsHostPtr, bool IsImplicit,
    bool UpdateRefCount, bool HasCloseModifier, bool DoNotAllocate) {
  void *rc = NULL;
  IsHostPtr = false;
  IsNew = false;
  DataMapMtx.lock();
  LookupResult lr = lookupMapping(HstPtrBegin, Size);

  // Check if the pointer is contained.
  // If a variable is mapped to the device manually by the user - which would
  // lead to the IsContained flag to be true - then we must ensure that the
  // device address is returned even under unified memory conditions.
  if (lr.Flags.IsContained ||
      ((lr.Flags.ExtendsBefore || lr.Flags.ExtendsAfter) && IsImplicit)) {
    auto &HT = *lr.Entry;
    IsNew = false;

    if (UpdateRefCount)
      HT.incRefCount();

    uintptr_t tp = HT.TgtPtrBegin + ((uintptr_t)HstPtrBegin - HT.HstPtrBegin);
    DP("Mapping exists%s with HstPtrBegin=" DPxMOD ", TgtPtrBegin=" DPxMOD ", "
        "Size=%ld,%s RefCount=%s\n", (IsImplicit ? " (implicit)" : ""),
        DPxPTR(HstPtrBegin), DPxPTR(tp), Size,
        (UpdateRefCount ? " updated" : ""),
        HT.isRefCountInf() ? "INF" : std::to_string(HT.getRefCount()).c_str());
    rc = (void *)tp;
  } else if ((lr.Flags.ExtendsBefore || lr.Flags.ExtendsAfter) && !IsImplicit) {
    // Explicit extension of mapped data - not allowed.
    DP("Explicit extension of mapping is not allowed.\n");
  } else if (Size) {
    // If unified shared memory is active, implicitly mapped variables that are not
    // privatized use host address. Any explicitly mapped variables also use
    // host address where correctness is not impeded. In all other cases
    // maps are respected.
    // In addition to the mapping rules above, the close map
    // modifier forces the mapping of the variable to the device.
    if (RTLs->RequiresFlags & OMP_REQ_UNIFIED_SHARED_MEMORY &&
        !HasCloseModifier) {
      DP("Return HstPtrBegin " DPxMOD " Size=%ld RefCount=%s\n",
         DPxPTR((uintptr_t)HstPtrBegin), Size, (UpdateRefCount ? " updated" : ""));
      IsHostPtr = true;
      rc = HstPtrBegin;
    } else if (DoNotAllocate) {
      DP("Mapping does not exist%s for HstPtrBegin=" DPxMOD ", Size=%ld\n",
         (IsImplicit ? " (implicit)" : ""), DPxPTR(HstPtrBegin), Size);
    } else {
      // If it is not contained and Size > 0 we should create a new entry for it.
      IsNew = true;
      uintptr_t tp = (uintptr_t)RTL->data_alloc(RTLDeviceID, Size, HstPtrBegin);
#if OMPT_SUPPORT
      // OpenMP 5.0 sec. 3.6.1 p. 398 L18:
      // "The target-data-allocation event occurs when a thread allocates data
      // on a target device."
      // OpenMP 5.0 sec. 3.6.6 p. 404 L32:
      // "The target-data-associate event occurs when a thread associates data
      // on a target device."
      // OpenMP 5.0 sec. 4.5.2.25 p. 489 L26-27:
      // "A registered ompt_callback_target_data_op callback is dispatched when
      // device memory is allocated or freed, as well as when data is copied to
      // or from a device."
      // The callbacks must dispatch after the allocation succeeds because they
      // require the device address.  Similarly, the callback for
      // ompt_target_data_associate should follow the callback for
      // ompt_target_data_alloc to reflect the order in which these events must
      // occur.
      if (OmptApi.ompt_get_enabled().ompt_callback_target_data_op) {
        // FIXME: We don't yet need the host_op_id and codeptr_ra arguments for
        // OpenACC support, so we haven't bothered to implement them yet.
        OmptApi.ompt_get_callbacks().ompt_callback(
            ompt_callback_target_data_op)(
            OmptApi.target_id, /*host_op_id*/ ompt_id_none,
            ompt_target_data_alloc, HstPtrBegin, HOST_DEVICE, (void *)tp,
            DeviceID, Size, /*codeptr_ra*/ NULL);
        OmptApi.ompt_get_callbacks().ompt_callback(
            ompt_callback_target_data_op)(
            OmptApi.target_id, /*host_op_id*/ ompt_id_none,
            ompt_target_data_associate, HstPtrBegin, HOST_DEVICE, (void *)tp,
            DeviceID, Size, /*codeptr_ra*/ NULL);
      }
#endif
      DP("Creating new map entry: HstBase=" DPxMOD ", HstBegin=" DPxMOD ", "
         "HstEnd=" DPxMOD ", TgtBegin=" DPxMOD "\n", DPxPTR(HstPtrBase),
         DPxPTR(HstPtrBegin), DPxPTR((uintptr_t)HstPtrBegin + Size), DPxPTR(tp));
      HostDataToTargetMap.push_front(HostDataToTargetTy((uintptr_t)HstPtrBase,
          (uintptr_t)HstPtrBegin, (uintptr_t)HstPtrBegin + Size, tp));
      rc = (void *)tp;
    }
  }

  DataMapMtx.unlock();
  return rc;
}

// Used by target_data_begin, target_data_end, target_data_update and target.
// Return the target pointer begin (where the data will be moved).
// Decrement the reference counter if called from target_data_end.
void *DeviceTy::getTgtPtrBegin(void *HstPtrBegin, int64_t Size, bool &IsLast,
    bool UpdateRefCount, bool &IsHostPtr, bool MustContain) {
  void *rc = NULL;
  IsHostPtr = false;
  IsLast = false;
  DataMapMtx.lock();
  LookupResult lr = lookupMapping(HstPtrBegin, Size);

  if (lr.Flags.IsContained ||
      (!MustContain && (lr.Flags.ExtendsBefore || lr.Flags.ExtendsAfter))) {
    auto &HT = *lr.Entry;
    IsLast = HT.getRefCount() == 1;

    if (!IsLast && UpdateRefCount)
      HT.decRefCount();

    uintptr_t tp = HT.TgtPtrBegin + ((uintptr_t)HstPtrBegin - HT.HstPtrBegin);
    DP("Mapping exists with HstPtrBegin=" DPxMOD ", TgtPtrBegin=" DPxMOD ", "
        "Size=%ld,%s RefCount=%s\n", DPxPTR(HstPtrBegin), DPxPTR(tp), Size,
        (UpdateRefCount ? " updated" : ""),
        HT.isRefCountInf() ? "INF" : std::to_string(HT.getRefCount()).c_str());
    rc = (void *)tp;
  } else if (RTLs->RequiresFlags & OMP_REQ_UNIFIED_SHARED_MEMORY) {
    // If the value isn't found in the mapping and unified shared memory
    // is on then it means we have stumbled upon a value which we need to
    // use directly from the host.
    DP("Get HstPtrBegin " DPxMOD " Size=%ld RefCount=%s\n",
       DPxPTR((uintptr_t)HstPtrBegin), Size, (UpdateRefCount ? " updated" : ""));
    IsHostPtr = true;
    rc = HstPtrBegin;
  }

  DataMapMtx.unlock();
  return rc;
}

// Return the target pointer begin (where the data will be moved).
// Lock-free version called when loading global symbols from the fat binary.
void *DeviceTy::getTgtPtrBegin(void *HstPtrBegin, int64_t Size) {
  uintptr_t hp = (uintptr_t)HstPtrBegin;
  LookupResult lr = lookupMapping(HstPtrBegin, Size);
  if (lr.Flags.IsContained || lr.Flags.ExtendsBefore || lr.Flags.ExtendsAfter) {
    auto &HT = *lr.Entry;
    uintptr_t tp = HT.TgtPtrBegin + (hp - HT.HstPtrBegin);
    return (void *)tp;
  }

  return NULL;
}

int DeviceTy::deallocTgtPtr(void *HstPtrBegin, int64_t Size, bool ForceDelete,
                            bool HasCloseModifier) {
  if (RTLs->RequiresFlags & OMP_REQ_UNIFIED_SHARED_MEMORY && !HasCloseModifier)
    return OFFLOAD_SUCCESS;
  // Check if the pointer is contained in any sub-nodes.
  int rc;
  DataMapMtx.lock();
  LookupResult lr = lookupMapping(HstPtrBegin, Size);
  if (lr.Flags.IsContained || lr.Flags.ExtendsBefore || lr.Flags.ExtendsAfter) {
    auto &HT = *lr.Entry;
    if (ForceDelete)
      HT.resetRefCount();
    if (HT.decRefCount() == 0) {
      DP("Deleting tgt data " DPxMOD " of size %ld\n",
          DPxPTR(HT.TgtPtrBegin), Size);
#if OMPT_SUPPORT
      // OpenMP 5.0 sec. 3.6.7 p. 405 L27:
      // "The target-data-disassociate event occurs when a thread disassociates
      // data on a target device."
      // OpenMP 5.0 sec. 3.6.2 p. 399 L19:
      // "The target-data-free event occurs when a thread frees data on a
      // target device."
      // OpenMP 5.0 sec. 4.5.2.25 p. 489 L26-27:
      // "A registered ompt_callback_target_data_op callback is dispatched when
      // device memory is allocated or freed, as well as when data is copied to
      // or from a device."
      // We assume the callbacks should dispatch before the free so that the
      // device address is still valid.  Similarly, we assume the callback for
      // ompt_target_data_disassociate should precede the callback for
      // ompt_target_data_delete to reflect the order in which these events
      // logically occur, even if that's not how the underlying actions are
      // coded here.  Moreover, this ordering is for symmetry with
      // ompt_target_data_alloc and ompt_target_data_associate.
      if (OmptApi.ompt_get_enabled().ompt_callback_target_data_op) {
        // FIXME: We don't yet need the host_op_id and codeptr_ra arguments for
        // OpenACC support, so we haven't bothered to implement them yet.
        OmptApi.ompt_get_callbacks().ompt_callback(
            ompt_callback_target_data_op)(
            OmptApi.target_id, /*host_op_id*/ ompt_id_none,
            ompt_target_data_disassociate, HstPtrBegin, HOST_DEVICE,
            (void *)HT.TgtPtrBegin, DeviceID, Size, /*codeptr_ra*/ NULL);
        OmptApi.ompt_get_callbacks().ompt_callback(
            ompt_callback_target_data_op)(
            OmptApi.target_id, /*host_op_id*/ ompt_id_none,
            ompt_target_data_delete, HstPtrBegin, HOST_DEVICE,
            (void *)HT.TgtPtrBegin, DeviceID, Size, /*codeptr_ra*/ NULL);
      }
#endif
      RTL->data_delete(RTLDeviceID, (void *)HT.TgtPtrBegin);
      DP("Removing%s mapping with HstPtrBegin=" DPxMOD ", TgtPtrBegin=" DPxMOD
          ", Size=%ld\n", (ForceDelete ? " (forced)" : ""),
          DPxPTR(HT.HstPtrBegin), DPxPTR(HT.TgtPtrBegin), Size);
      HostDataToTargetMap.erase(lr.Entry);
    }
    rc = OFFLOAD_SUCCESS;
  } else {
    DP("Section to delete (hst addr " DPxMOD ") does not exist in the allocated"
       " memory\n", DPxPTR(HstPtrBegin));
    rc = OFFLOAD_FAIL;
  }

  DataMapMtx.unlock();
  return rc;
}

/// Init device, should not be called directly.
void DeviceTy::init() {
#if OMPT_SUPPORT
  // FIXME: Is this the right place for this event?  Should it include global
  // data mapping in CheckDeviceAndCtors in omptarget.cpp?
  if (OmptApi.ompt_get_enabled().ompt_callback_device_initialize_start) {
    // FIXME: Lots of missing info is needed here.
    OmptApi.ompt_get_callbacks().ompt_callback(
        ompt_callback_device_initialize_start)(
        /*device_num*/ DeviceID,
        /*type*/ "<device type tracking is not yet implemented>",
        /*device*/ NULL,
        /*lookup*/ NULL,
        /*documentation*/ NULL);
  }
#endif
  // Make call to init_requires if it exists for this plugin.
  if (RTL->init_requires)
    RTL->init_requires(RTLs->RequiresFlags);
  int32_t rc = RTL->init_device(RTLDeviceID);
  if (rc == OFFLOAD_SUCCESS) {
    IsInit = true;
  }
#if OMPT_SUPPORT
  // OpenMP 5.0 sec. 2.12.1 p. 160 L3-7:
  // "The device-initialize event occurs in a thread that encounters the first
  // target, target data, or target enter data construct or a device memory
  // routine that is associated with a particular target device after the
  // thread initiates initialization of OpenMP on the device and the device's
  // OpenMP initialization, which may include device-side tool initialization,
  // completes."
  // OpenMP 5.0 sec. 4.5.2.19 p. 482 L24-25:
  // "The OpenMP implementation invokes this callback after OpenMP is
  // initialized for the device but before execution of any OpenMP construct is
  // started on the device."
  if (OmptApi.ompt_get_enabled().ompt_callback_device_initialize) {
    // FIXME: Lots of missing info is needed here.
    OmptApi.ompt_get_callbacks().ompt_callback(ompt_callback_device_initialize)(
        /*device_num*/ DeviceID,
        /*type*/ "<device type tracking is not yet implemented>",
        /*device*/ NULL,
        /*lookup*/ NULL,
        /*documentation*/ NULL);
  }
  if (OmptApi.ompt_get_enabled().enabled)
    ompt_record_device_init(DeviceID);
#endif
}

/// Thread-safe method to initialize the device only once.
int32_t DeviceTy::initOnce() {
  std::call_once(InitFlag, &DeviceTy::init, this);

  // At this point, if IsInit is true, then either this thread or some other
  // thread in the past successfully initialized the device, so we can return
  // OFFLOAD_SUCCESS. If this thread executed init() via call_once() and it
  // failed, return OFFLOAD_FAIL. If call_once did not invoke init(), it means
  // that some other thread already attempted to execute init() and if IsInit
  // is still false, return OFFLOAD_FAIL.
  if (IsInit)
    return OFFLOAD_SUCCESS;
  else
    return OFFLOAD_FAIL;
}

// Load binary to device.
__tgt_target_table *DeviceTy::load_binary(void *Img) {
  RTL->Mtx.lock();
  __tgt_target_table *rc = RTL->load_binary(RTLDeviceID, Img);
  RTL->Mtx.unlock();
  return rc;
}

// Submit data to device
int32_t DeviceTy::data_submit(void *TgtPtrBegin, void *HstPtrBegin,
                              int64_t Size, __tgt_async_info *AsyncInfoPtr) {
#if OMPT_SUPPORT
  // OpenMP 5.0 sec. 2.19.7.1 p. 321 L15:
  // "The target-data-op event occurs when a thread initiates a data operation
  // on a target device."
  // OpenMP 5.0 sec. 3.6.4 p. 401 L24:
  // "The target-data-op event occurs when a thread transfers data on a target
  // device."
  // OpenMP 5.0 sec. 4.5.2.25 p. 489 L26-27:
  // "A registered ompt_callback_target_data_op callback is dispatched when
  // device memory is allocated or freed, as well as when data is copied to or
  // from a device."
  if (OmptApi.ompt_get_enabled().ompt_callback_target_data_op) {
    // FIXME: We don't yet need the host_op_id and codeptr_ra arguments for
    // OpenACC support, so we haven't bothered to implement them yet.
    OmptApi.ompt_get_callbacks().ompt_callback(ompt_callback_target_data_op)(
        OmptApi.target_id, /*host_op_id*/ ompt_id_none,
        ompt_target_data_transfer_to_device, HstPtrBegin, HOST_DEVICE,
        TgtPtrBegin, DeviceID, Size, /*codeptr_ra*/ NULL);
  }
#endif
  if (!AsyncInfoPtr || !RTL->data_submit_async || !RTL->synchronize)
    return RTL->data_submit(RTLDeviceID, TgtPtrBegin, HstPtrBegin, Size);
  else
    return RTL->data_submit_async(RTLDeviceID, TgtPtrBegin, HstPtrBegin, Size,
                                  AsyncInfoPtr);
}

// Retrieve data from device
int32_t DeviceTy::data_retrieve(void *HstPtrBegin, void *TgtPtrBegin,
                                int64_t Size, __tgt_async_info *AsyncInfoPtr) {
#if OMPT_SUPPORT
  // OpenMP 5.0 sec. 2.19.7.1 p. 321 L15:
  // "The target-data-op event occurs when a thread initiates a data operation
  // on a target device."
  // OpenMP 5.0 sec. 3.6.4 p. 401 L24:
  // "The target-data-op event occurs when a thread transfers data on a target
  // device."
  // OpenMP 5.0 sec. 4.5.2.25 p. 489 L26-27:
  // "A registered ompt_callback_target_data_op callback is dispatched when
  // device memory is allocated or freed, as well as when data is copied to or
  // from a device."
  if (OmptApi.ompt_get_enabled().ompt_callback_target_data_op) {
    // FIXME: We don't yet need the host_op_id and codeptr_ra arguments for
    // OpenACC support, so we haven't bothered to implement them yet.
    OmptApi.ompt_get_callbacks().ompt_callback(ompt_callback_target_data_op)(
        OmptApi.target_id, /*host_op_id*/ ompt_id_none,
        ompt_target_data_transfer_from_device, TgtPtrBegin, DeviceID,
        HstPtrBegin, HOST_DEVICE, Size, /*codeptr_ra*/ NULL);
  }
#endif
  if (!AsyncInfoPtr || !RTL->data_retrieve_async || !RTL->synchronize)
    return RTL->data_retrieve(RTLDeviceID, HstPtrBegin, TgtPtrBegin, Size);
  else
    return RTL->data_retrieve_async(RTLDeviceID, HstPtrBegin, TgtPtrBegin, Size,
                                    AsyncInfoPtr);
}

// Run region on device
int32_t DeviceTy::run_region(void *TgtEntryPtr, void **TgtVarsPtr,
                             ptrdiff_t *TgtOffsets, int32_t TgtVarsSize,
                             __tgt_async_info *AsyncInfoPtr) {
  if (!AsyncInfoPtr || !RTL->run_region || !RTL->synchronize)
    return RTL->run_region(RTLDeviceID, TgtEntryPtr, TgtVarsPtr, TgtOffsets,
                           TgtVarsSize OMPT_SUPPORT_IF(, &OmptApi));
  else
    return RTL->run_region_async(RTLDeviceID, TgtEntryPtr, TgtVarsPtr,
                                 TgtOffsets, TgtVarsSize, AsyncInfoPtr
                                 OMPT_SUPPORT_IF(, &OmptApi));
}

// Run team region on device.
int32_t DeviceTy::run_team_region(void *TgtEntryPtr, void **TgtVarsPtr,
                                  ptrdiff_t *TgtOffsets, int32_t TgtVarsSize,
                                  int32_t NumTeams, int32_t ThreadLimit,
                                  uint64_t LoopTripCount,
                                  __tgt_async_info *AsyncInfoPtr) {
  if (!AsyncInfoPtr || !RTL->run_team_region_async || !RTL->synchronize)
    return RTL->run_team_region(RTLDeviceID, TgtEntryPtr, TgtVarsPtr,
                                TgtOffsets, TgtVarsSize, NumTeams, ThreadLimit,
                                LoopTripCount OMPT_SUPPORT_IF(, &OmptApi));
  else
    return RTL->run_team_region_async(
        RTLDeviceID, TgtEntryPtr, TgtVarsPtr, TgtOffsets, TgtVarsSize, NumTeams,
        ThreadLimit, LoopTripCount, AsyncInfoPtr OMPT_SUPPORT_IF(, &OmptApi));
}

/// Check whether a device has an associated RTL and initialize it if it's not
/// already initialized.
bool device_is_ready(int device_num) {
  DP("Checking whether device %d is ready.\n", device_num);
  // Devices.size() can only change while registering a new
  // library, so try to acquire the lock of RTLs' mutex.
  RTLsMtx->lock();
  size_t Devices_size = Devices.size();
  RTLsMtx->unlock();
  if (Devices_size <= (size_t)device_num) {
    DP("Device ID  %d does not have a matching RTL\n", device_num);
    return false;
  }

  // Get device info
  DeviceTy &Device = Devices[device_num];

  DP("Is the device %d (local ID %d) initialized? %d\n", device_num,
       Device.RTLDeviceID, Device.IsInit);

  // Init the device if not done before
  if (!Device.IsInit && Device.initOnce() != OFFLOAD_SUCCESS) {
    DP("Failed to init device %d\n", device_num);
    return false;
  }

  DP("Device %d is ready to use.\n", device_num);

  return true;
}
