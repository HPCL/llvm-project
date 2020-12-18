//===----------- device.h - Target independent OpenMP target RTL ----------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// Declarations for managing devices that are handled by RTL plugins.
//
//===----------------------------------------------------------------------===//

#ifndef _OMPTARGET_DEVICE_H
#define _OMPTARGET_DEVICE_H

#define OMPT_FOR_LIBOMPTARGET
#include "../../runtime/src/ompt-internal.h"

#include <cassert>
#include <cstddef>
#include <list>
#include <map>
#include <mutex>
#include <set>
#include <vector>

// Forward declarations.
struct RTLInfoTy;
struct __tgt_bin_desc;
struct __tgt_target_table;
struct __tgt_async_info;

/// Map between host data and target data.
struct HostDataToTargetTy {
  uintptr_t HstPtrBase; // host info.
  uintptr_t HstPtrBegin;
  uintptr_t HstPtrEnd; // non-inclusive.

  uintptr_t TgtPtrBegin; // target info.

private:
  /// use mutable to allow modification via std::set iterator which is const.
  ///@{
  mutable uint64_t RefCount;
  mutable uint64_t HoldRefCount;
  ///@}
  static const uint64_t INFRefCount = ~(uint64_t)0;

public:
  HostDataToTargetTy(uintptr_t BP, uintptr_t B, uintptr_t E, uintptr_t TB,
                     bool UseHoldRefCount, bool IsINF)
      : HstPtrBase(BP), HstPtrBegin(B), HstPtrEnd(E), TgtPtrBegin(TB),
        RefCount(IsINF ? INFRefCount : !UseHoldRefCount),
        HoldRefCount(UseHoldRefCount) {}

  /// Get the total reference count.
  uint64_t getRefCount() const {
    if (RefCount == INFRefCount)
      return RefCount;
    return RefCount + HoldRefCount;
  }

  /// Get a specific reference count.
  uint64_t getRefCount(bool UseHoldRefCount) const {
    return UseHoldRefCount ? HoldRefCount : RefCount;
  }

  /// Reset the dynamic reference count only and return the total reference
  /// count.
  uint64_t resetRefCount() const {
    if (RefCount != INFRefCount)
      RefCount = 1;

    return getRefCount();
  }

  /// Increment the specified reference count and return the total reference
  /// count.
  uint64_t incRefCount(bool UseHoldRefCount) const {
    if (UseHoldRefCount) {
      ++HoldRefCount;
      assert(HoldRefCount < INFRefCount && "hold refcount overflow");
    } else if (RefCount != INFRefCount) {
      ++RefCount;
      assert(RefCount < INFRefCount && "refcount overflow");
    }

    return getRefCount();
  }

  /// Decrement the specified reference count and return the total reference
  /// count.
  uint64_t decRefCount(bool UseHoldRefCount) const {
    if (UseHoldRefCount) {
      assert(HoldRefCount > 0 && "hold refcount underflow");
      --HoldRefCount;
    } else if (RefCount != INFRefCount) {
      assert(RefCount > 0 && "refcount underflow");
      --RefCount;
    }

    return getRefCount();
  }

  /// Is the dynamic (and thus total) reference count infinite?
  bool isRefCountInf() const {
    return RefCount == INFRefCount;
  }
};

typedef uintptr_t HstPtrBeginTy;
inline bool operator<(const HostDataToTargetTy &lhs, const HstPtrBeginTy &rhs) {
  return lhs.HstPtrBegin < rhs;
}
inline bool operator<(const HstPtrBeginTy &lhs, const HostDataToTargetTy &rhs) {
  return lhs < rhs.HstPtrBegin;
}
inline bool operator<(const HostDataToTargetTy &lhs,
                      const HostDataToTargetTy &rhs) {
  return lhs.HstPtrBegin < rhs.HstPtrBegin;
}

typedef std::set<HostDataToTargetTy, std::less<>> HostDataToTargetListTy;

struct LookupResult {
  struct {
    unsigned IsContained   : 1;
    unsigned ExtendsBefore : 1;
    unsigned ExtendsAfter  : 1;
  } Flags;

  HostDataToTargetListTy::iterator Entry;

  LookupResult() : Flags({0,0,0}), Entry() {}
};

/// Map for shadow pointers
struct ShadowPtrValTy {
  void *HstPtrVal;
  void *TgtPtrAddr;
  void *TgtPtrVal;
};
typedef std::map<void *, ShadowPtrValTy> ShadowPtrListTy;

///
struct PendingCtorDtorListsTy {
  std::list<void *> PendingCtors;
  std::list<void *> PendingDtors;
};
typedef std::map<__tgt_bin_desc *, PendingCtorDtorListsTy>
    PendingCtorsDtorsPerLibrary;

struct DeviceTy {
  int32_t DeviceID;
  RTLInfoTy *RTL;
  int32_t RTLDeviceID;
#if OMPT_SUPPORT
  ompt_plugin_api_t OmptApi;
#endif

  bool IsInit;
  std::once_flag InitFlag;
  bool HasPendingGlobals;

  HostDataToTargetListTy HostDataToTargetMap;
  PendingCtorsDtorsPerLibrary PendingCtorsDtors;

  ShadowPtrListTy ShadowPtrMap;

  std::mutex DataMapMtx, PendingGlobalsMtx, ShadowMtx;

  // NOTE: Once libomp gains full target-task support, this state should be
  // moved into the target task in libomp.
  std::map<int32_t, uint64_t> LoopTripCnt;

  DeviceTy(RTLInfoTy *RTL)
      : DeviceID(-1), RTL(RTL), RTLDeviceID(-1), IsInit(false), InitFlag(),
        HasPendingGlobals(false), HostDataToTargetMap(), PendingCtorsDtors(),
        ShadowPtrMap(), DataMapMtx(), PendingGlobalsMtx(), ShadowMtx() {
#if OMPT_SUPPORT
    OmptApi.target_id = ompt_id_none;
    OmptApi.global_device_id = DeviceID;
    OmptApi.ompt_get_enabled = ompt_get_enabled;
    OmptApi.ompt_get_callbacks = ompt_get_callbacks;
#endif
  }

  // The existence of mutexes makes DeviceTy non-copyable. We need to
  // provide a copy constructor and an assignment operator explicitly.
  DeviceTy(const DeviceTy &d)
      : DeviceID(d.DeviceID), RTL(d.RTL), RTLDeviceID(d.RTLDeviceID),
        OMPT_SUPPORT_IF(OmptApi(d.OmptApi),) IsInit(d.IsInit), InitFlag(),
        HasPendingGlobals(d.HasPendingGlobals),
        HostDataToTargetMap(d.HostDataToTargetMap),
        PendingCtorsDtors(d.PendingCtorsDtors), ShadowPtrMap(d.ShadowPtrMap),
        DataMapMtx(), PendingGlobalsMtx(), ShadowMtx(),
        LoopTripCnt(d.LoopTripCnt) {}

  DeviceTy& operator=(const DeviceTy &d) {
    DeviceID = d.DeviceID;
    RTL = d.RTL;
    RTLDeviceID = d.RTLDeviceID;
#if OMPT_SUPPORT
    OmptApi = d.OmptApi;
#endif
    IsInit = d.IsInit;
    HasPendingGlobals = d.HasPendingGlobals;
    HostDataToTargetMap = d.HostDataToTargetMap;
    PendingCtorsDtors = d.PendingCtorsDtors;
    ShadowPtrMap = d.ShadowPtrMap;
    LoopTripCnt = d.LoopTripCnt;

    return *this;
  }

  // Return true if data can be copied to DstDevice directly
  bool isDataExchangable(const DeviceTy& DstDevice);

  uint64_t getMapEntryRefCnt(void *HstPtrBegin);
  LookupResult lookupMapping(void *HstPtrBegin, int64_t Size);
  size_t getAccessibleBuffer(void *Ptr, int64_t Size, void **BufferHost,
                             void **BufferDevice);
  void *getOrAllocTgtPtr(void *HstPtrBegin, void *HstPtrBase, int64_t Size,
                         bool &IsNew, bool &IsHostPtr, bool IsImplicit,
                         bool UpdateRefCount, bool HasCloseModifier,
                         bool HasPresentModifier, bool HasNoAllocModifier,
                         bool HasHoldModifier);
  void *getTgtPtrBegin(void *HstPtrBegin, int64_t Size);
  void *getTgtPtrBegin(void *HstPtrBegin, int64_t Size, bool &IsLast,
                       bool UpdateRefCount, bool UseHoldRefCount,
                       bool &IsHostPtr, bool MustContain = false,
                       bool HasDeleteModifier = false);
  int deallocTgtPtr(void *TgtPtrBegin, int64_t Size, bool ForceDelete,
                    bool HasCloseModifier, bool HasHoldModifier);
  int associatePtr(void *HstPtrBegin, void *TgtPtrBegin, int64_t Size);
  int disassociatePtr(void *HstPtrBegin, void *&TgtPtrBegin, int64_t &Size);

  // calls to RTL
  int32_t initOnce();
  __tgt_target_table *load_binary(void *Img);

  // device memory allocation/deallocation routines
  /// Allocates \p Size bytes on the device and returns the address/nullptr when
  /// succeeds/fails. \p HstPtr is an address of the host data which the
  /// allocated target data will be associated with. If it is unknown, the
  /// default value of \p HstPtr is nullptr. Note: this function doesn't do
  /// pointer association. Actually, all the __tgt_rtl_data_alloc
  /// implementations ignore \p HstPtr.
  void *allocData(int64_t Size, void *HstPtr = nullptr);
  /// Deallocates memory which \p TgtPtrBegin points at and returns
  /// OFFLOAD_SUCCESS/OFFLOAD_FAIL when succeeds/fails.
  int32_t deleteData(void *TgtPtrBegin);

  // Data transfer. When AsyncInfoPtr is nullptr, the transfer will be
  // synchronous.
  // Copy data from host to device
  int32_t submitData(void *TgtPtrBegin, void *HstPtrBegin, int64_t Size,
                     __tgt_async_info *AsyncInfoPtr);
  // Copy data from device back to host
  int32_t retrieveData(void *HstPtrBegin, void *TgtPtrBegin, int64_t Size,
                       __tgt_async_info *AsyncInfoPtr);
  // Copy data from current device to destination device directly
  int32_t data_exchange(void *SrcPtr, DeviceTy DstDev, void *DstPtr,
                        int64_t Size, __tgt_async_info *AsyncInfoPtr);

  int32_t runRegion(void *TgtEntryPtr, void **TgtVarsPtr, ptrdiff_t *TgtOffsets,
                    int32_t TgtVarsSize, __tgt_async_info *AsyncInfoPtr);
  int32_t runTeamRegion(void *TgtEntryPtr, void **TgtVarsPtr,
                        ptrdiff_t *TgtOffsets, int32_t TgtVarsSize,
                        int32_t NumTeams, int32_t ThreadLimit,
                        uint64_t LoopTripCount, __tgt_async_info *AsyncInfoPtr);

  /// Synchronize device/queue/event based on \p AsyncInfoPtr and return
  /// OFFLOAD_SUCCESS/OFFLOAD_FAIL when succeeds/fails.
  int32_t synchronize(__tgt_async_info *AsyncInfoPtr);

private:
  // Call to RTL
  void init(); // To be called only via DeviceTy::initOnce()
};

/// Map between Device ID (i.e. openmp device id) and its DeviceTy.
typedef std::vector<DeviceTy> DevicesTy;
extern DevicesTy Devices;

extern bool device_is_ready(int device_num);

#endif
