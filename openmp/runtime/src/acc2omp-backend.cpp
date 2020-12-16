/*
 * acc2omp-backend.cpp -- libacc2omp backend for LLVM's OpenMP runtime.
 */

//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "kmp.h"

#include <cstdarg>
#include <cstdlib>

#include "acc2omp-backend.h"

static kmp_i18n_id_t acc2omp_msg_to_llvm(acc2omp_msgid_t MsgId) {
  switch (MsgId) {
  case acc2omp_msg_alloc_fail:
    return kmp_i18n_msg_MemoryAllocFailed;
  case acc2omp_msg_acc_proflib_fail:
    return kmp_i18n_msg_AccProflibFail;
  case acc2omp_msg_unsupported_event_register:
    return kmp_i18n_msg_AccUnsupportedEventRegister;
  case acc2omp_msg_unsupported_event_unregister:
    return kmp_i18n_msg_AccUnsupportedEventUnregister;
  case acc2omp_msg_event_unsupported_toggle:
    return kmp_i18n_msg_AccEventUnsupportedToggle;
  case acc2omp_msg_event_unsupported_multiple_register:
    return kmp_i18n_msg_AccEventUnsupportedMultipleRegister;
  case acc2omp_msg_event_register_result:
    return kmp_i18n_msg_AccEventRegisterResult;
  case acc2omp_msg_event_unregister_result:
    return kmp_i18n_msg_AccEventUnregisterResult;
  case acc2omp_msg_event_unregister_unregistered:
    return kmp_i18n_msg_AccEventUnregisterUnregistered;
  case acc2omp_msg_callback_unregister_unregistered:
    return kmp_i18n_msg_AccCallbackUnregisterUnregistered;
  case acc2omp_msg_map_data_host_pointer_null:
    return kmp_i18n_msg_AccMapDataHostPointerNull;
  case acc2omp_msg_map_data_device_pointer_null:
    return kmp_i18n_msg_AccMapDataDevicePointerNull;
  case acc2omp_msg_map_data_bytes_zero:
    return kmp_i18n_msg_AccMapDataBytesZero;
  case acc2omp_msg_map_data_already_present:
    return kmp_i18n_msg_AccMapDataAlreadyPresent;
  case acc2omp_msg_map_data_fail:
    return kmp_i18n_msg_AccMapDataFail;
  case acc2omp_msg_unmap_data_pointer_null:
    return kmp_i18n_msg_AccUnmapDataPointerNull;
  case acc2omp_msg_unmap_data_fail:
    return kmp_i18n_msg_AccUnmapDataFail;
  }
  KMP_ASSERT2(0, "unexpected acc2omp_msg_t");
  return kmp_i18n_null;
}

void acc2omp_warn(acc2omp_msg_t Msg, ...) {
  va_list Args;
  va_start(Args, Msg);
  kmp_msg_t KmpMsg = __kmp_msg_vformat(acc2omp_msg_to_llvm(Msg.Id), Args);
  va_end(Args);
  __kmp_msg(kmp_ms_warning, KmpMsg, __kmp_msg_null);
}

void acc2omp_fatal(acc2omp_msg_t Msg, ...) {
  va_list Args;
  va_start(Args, Msg);
  kmp_msg_t KmpMsg = __kmp_msg_vformat(acc2omp_msg_to_llvm(Msg.Id), Args);
  va_end(Args);
  __kmp_fatal(KmpMsg, __kmp_msg_null);
}

void acc2omp_assert(int Cond, const char *Msg, const char *File, int Line) {
  // The cmake variable LLVM_ENABLE_ASSERTIONS controls both whether libacc2omp
  // ever calls acc2omp_assert and also whether KMP_ASSERT4 is a no-op, so we
  // need not implement a fallback assertion mechanism here as discussed in
  // acc2omp_assert documentation in acc2omp-backend.h
  KMP_ASSERT4(Cond, Msg, File, Line);
}
