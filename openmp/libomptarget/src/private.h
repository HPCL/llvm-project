//===---------- private.h - Target independent OpenMP target RTL ----------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// Private function declarations and helper macros for debugging output.
//
//===----------------------------------------------------------------------===//

#ifndef _OMPTARGET_PRIVATE_H
#define _OMPTARGET_PRIVATE_H

#include <omptarget.h>
#include <Debug.h>

#include <cstdint>

extern int targetDataBegin(DeviceTy &Device, int32_t arg_num, void **args_base,
                           void **args, int64_t *arg_sizes, int64_t *arg_types,
                           void **arg_mappers,
                           __tgt_async_info *async_info_ptr);

extern int targetDataEnd(DeviceTy &Device, int32_t ArgNum, void **ArgBases,
                         void **Args, int64_t *ArgSizes, int64_t *ArgTypes,
                         void **ArgMappers, __tgt_async_info *AsyncInfo);

extern int target_data_update(DeviceTy &Device, int32_t arg_num,
                              void **args_base, void **args,
                              int64_t *arg_sizes, int64_t *arg_types,
                              void **arg_mappers,
                              __tgt_async_info *async_info_ptr = nullptr);

extern int target(int64_t DeviceId, void *HostPtr, int32_t ArgNum,
                  void **ArgBases, void **Args, int64_t *ArgSizes,
                  int64_t *ArgTypes, void **ArgMappers, int32_t TeamNum,
                  int32_t ThreadLimit, int IsTeamConstruct);

extern int CheckDeviceAndCtors(int64_t device_id);

#ifdef OMPTARGET_DEBUG
extern const char *deviceTypeToString(omp_device_t DevType);
#endif

// This structure stores information of a mapped memory region.
struct MapComponentInfoTy {
  void *Base;
  void *Begin;
  int64_t Size;
  int64_t Type;
  MapComponentInfoTy() = default;
  MapComponentInfoTy(void *Base, void *Begin, int64_t Size, int64_t Type)
      : Base(Base), Begin(Begin), Size(Size), Type(Type) {}
};

// This structure stores all components of a user-defined mapper. The number of
// components are dynamically decided, so we utilize C++ STL vector
// implementation here.
struct MapperComponentsTy {
  std::vector<MapComponentInfoTy> Components;
  int32_t size() { return Components.size(); }
};

// The mapper function pointer type. It follows the signature below:
// void .omp_mapper.<type_name>.<mapper_id>.(void *rt_mapper_handle,
//                                           void *base, void *begin,
//                                           size_t size, int64_t type);
typedef void (*MapperFuncPtrTy)(void *, void *, void *, int64_t, int64_t);

// Function pointer type for target_data_* functions (targetDataBegin,
// targetDataEnd and target_data_update).
typedef int (*TargetDataFuncPtrTy)(DeviceTy &, int32_t, void **, void **,
                                   int64_t *, int64_t *, void **,
                                   __tgt_async_info *);

// Implemented in libomp, they are called from within __tgt_* functions.
#ifdef __cplusplus
extern "C" {
#endif
// functions that extract info from libomp; keep in sync
int omp_get_default_device(void) __attribute__((weak));
int32_t __kmpc_omp_taskwait(void *loc_ref, int32_t gtid) __attribute__((weak));
int32_t __kmpc_global_thread_num(void *) __attribute__((weak));
int __kmpc_get_target_offload(void) __attribute__((weak));
void __kmpc_set_directive_info(int kind, int is_explicit_event,
                               const char *src_file, const char *func_name,
                               int line_no, int end_line_no, int func_line_no,
                               int func_end_line_no) __attribute__((weak));
void __kmpc_clear_directive_info() __attribute__((weak));
#ifdef __cplusplus
}
#endif

#define TARGET_NAME Libomptarget
#define DEBUG_PREFIX GETNAME(TARGET_NAME)

////////////////////////////////////////////////////////////////////////////////
/// dump a table of all the host-target pointer pairs on failure
static inline void dumpTargetPointerMappings(const DeviceTy &Device) {
  if (Device.HostDataToTargetMap.empty())
    return;

  fprintf(stderr, "Device %d Host-Device Pointer Mappings:\n", Device.DeviceID);
  fprintf(stderr, "%-18s %-18s %s\n", "Host Ptr", "Target Ptr", "Size (B)");
  for (const auto &HostTargetMap : Device.HostDataToTargetMap) {
    fprintf(stderr, DPxMOD " " DPxMOD " %lu\n",
            DPxPTR(HostTargetMap.HstPtrBegin),
            DPxPTR(HostTargetMap.TgtPtrBegin),
            HostTargetMap.HstPtrEnd - HostTargetMap.HstPtrBegin);
  }
}

#if OMPT_SUPPORT
void ompt_dispatch_callback_target(
    ompt_target_t kind, ompt_scope_endpoint_t endpoint, DeviceTy &Device);
#endif

#endif
