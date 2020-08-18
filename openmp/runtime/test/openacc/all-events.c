// Check that all callbacks are dispatched in the correct order with the
// correct data for a simple program.
//
// Check the following cases:
// - A data construct by itself (-DDIR=DIR_DATA)
// - A parallel construct by itself (-DDIR=DIR_PAR)
// - A parallel construct and update directive nested within data constructs
//   (-DDIR=DIR_DATAPAR)

// REQUIRES: ompt
//
// RUN: %data dirs {
// RUN:   (dir-cflags=-DDIR=DIR_DATA dir-fc1=DATA dir-fc2=NOPAR dir-fc3=NODATAPAR
// RUN:    arr-construct=data
// RUN:    arr0-line-no=20000 arr0-end-line-no=30000
// RUN:    arr1-line-no=20000 arr1-end-line-no=30000
// RUN:    kern-line-no=      kern-end-line-no=
// RUN:    update0-line-no=   update1-line-no=      )
// RUN:   (dir-cflags=-DDIR=DIR_PAR dir-fc1=PAR dir-fc2=HASPAR dir-fc3=NODATAPAR
// RUN:    arr-construct=parallel
// RUN:    arr0-line-no=40000 arr0-end-line-no=80000
// RUN:    arr1-line-no=40000 arr1-end-line-no=80000
// RUN:    kern-line-no=40000 kern-end-line-no=80000
// RUN:    update0-line-no=   update1-line-no=      )
// RUN:   (dir-cflags=-DDIR=DIR_DATAPAR dir-fc1=DATAPAR dir-fc2=HASPAR dir-fc3=HASDATAPAR
// RUN:    arr-construct=data
// RUN:    arr0-line-no=50000    arr0-end-line-no=120000
// RUN:    arr1-line-no=60000    arr1-end-line-no=110000
// RUN:    kern-line-no=70000    kern-end-line-no=80000
// RUN:    update0-line-no=90000 update1-line-no=100000 )
// RUN: }
// RUN: %data tgts {
// RUN:   (run-if=
// RUN:    tgt-host-or-off=HOST
// RUN:    tgt-cflags=
// RUN:    tgt-fc=HOST,HOST-%[dir-fc1],HOST-%[dir-fc2],HOST-%[dir-fc3])
// RUN:   (run-if=%run-if-x86_64
// RUN:    tgt-host-or-off=OFF
// RUN:    tgt-cflags=-fopenmp-targets=%run-x86_64-triple
// RUN:    tgt-fc=OFF,OFF-%[dir-fc1],OFF-%[dir-fc2],OFF-%[dir-fc3],X86_64,X86_64-%[dir-fc1],X86_64-%[dir-fc2],X86_64-%[dir-fc3])
// RUN:   (run-if=%run-if-ppc64le
// RUN:    tgt-host-or-off=OFF
// RUN:    tgt-cflags=-fopenmp-targets=%run-ppc64le-triple
// RUN:    tgt-fc=OFF,OFF-%[dir-fc1],OFF-%[dir-fc2],OFF-%[dir-fc3],PPC64LE,PPC64LE-%[dir-fc1],PPC64LE-%[dir-fc2],PPC64LE-%[dir-fc3])
// RUN:   (run-if=%run-if-nvptx64
// RUN:    tgt-host-or-off=OFF
// RUN:    tgt-cflags='-fopenmp-targets=%run-nvptx64-triple -DNVPTX64'
// RUN:    tgt-fc=OFF,OFF-%[dir-fc1],OFF-%[dir-fc2],OFF-%[dir-fc3],NVPTX64,NVPTX64-%[dir-fc1],NVPTX64-%[dir-fc2],NVPTX64-%[dir-fc3])
// RUN: }
//      # Check offloading compilation both with and without offloading at run
//      # time.  This is important because some runtime calls that must be
//      # instrumented with some callback data are not exercised in both cases.
// RUN: %data run-envs {
// RUN:   (run-env=
// RUN:    env-fc=%[tgt-fc],%[tgt-host-or-off]-BEFORE-ENV,%[tgt-host-or-off]-BEFORE-ENV-%[dir-fc2])
// RUN:   (run-env='env OMP_TARGET_OFFLOAD=disabled'
// RUN:    env-fc=HOST,HOST-%[dir-fc1],HOST-%[dir-fc2],%[tgt-host-or-off]-BEFORE-ENV,%[tgt-host-or-off]-BEFORE-ENV-%[dir-fc2])
// RUN: }
// RUN: %for dirs {
// RUN:   %for tgts {
// RUN:     %[run-if] %clang -Xclang -verify -fopenacc %flags %s -o %t \
// RUN:                      %[tgt-cflags] %[dir-cflags]
// RUN:     %for run-envs {
// RUN:       %[run-if] %[run-env] %t > %t.out 2> %t.err
// RUN:       %[run-if] FileCheck -input-file %t.err %s \
// RUN:           -allow-empty -check-prefixes=ERR
// RUN:       %[run-if] FileCheck -input-file %t.out %s \
// RUN:           -match-full-lines -strict-whitespace \
// RUN:           -check-prefixes=CHECK,CHECK-%[dir-fc1],CHECK-%[dir-fc2],CHECK-%[dir-fc3],%[env-fc] \
// RUN:           -DVERSION=%acc-version -DHOST_DEV=%acc-host-dev \
// RUN:           -DOFF_DEV=0 -DTHREAD_ID=0 -DASYNC_QUEUE=-1 -DSRC_FILE=%s \
// RUN:           -DARR_CONSTRUCT=%[arr-construct] \
// RUN:           -DARR0_LINE_NO=%[arr0-line-no] -DARR0_END_LINE_NO=%[arr0-end-line-no] \
// RUN:           -DARR1_LINE_NO=%[arr1-line-no] -DARR1_END_LINE_NO=%[arr1-end-line-no] \
// RUN:           -DKERN_LINE_NO=%[kern-line-no] -DKERN_END_LINE_NO=%[kern-end-line-no] \
// RUN:           -DUPDATE0_LINE_NO=%[update0-line-no] -DUPDATE1_LINE_NO=%[update1-line-no] \
// RUN:           -DFUNC_LINE_NO=10000 -DFUNC_END_LINE_NO=130000
// RUN:     }
// RUN:   }
// RUN: }
//
// END.

// expected-no-diagnostics

#include "callbacks.h"
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#define DIR_DATA     1
#define DIR_PAR      2
#define DIR_DATAPAR  3

#ifndef NVPTX64
# define NVPTX64 0
#endif

// ERR-NOT:{{.}}
void acc_register_library(acc_prof_reg reg, acc_prof_reg unreg,
                          acc_prof_lookup lookup) {
  register_all_callbacks(reg);
}

#line 10000
int main() {
  const char *ompTargetOffload = getenv("OMP_TARGET_OFFLOAD");
  bool offloadDisabled = ompTargetOffload && !strcmp(ompTargetOffload,
                                                     "disabled");

  // CHECK-NOT:{{.}}

  int arr0[2] = {10, 11};
  int arr1[5] = {20, 21, 22, 23, 24};
  int notPresent0[10];
  int notPresent1[11];

  //      CHECK:arr0 host ptr = [[ARR0_HOST_PTR:0x[a-z0-9]+]]
  // CHECK-NEXT:arr1 host ptr = [[ARR1_HOST_PTR:0x[a-z0-9]+]]
  printf("arr0 host ptr = %p\n", arr0);
  printf("arr1 host ptr = %p\n", arr1);

  // CHECK-NEXT:before kernel
  printf("before kernel\n");

  // Device initialization.
  //
  // OFF-NEXT:acc_ev_device_init_start
  // OFF-NEXT:  acc_prof_info
  // OFF-NEXT:    event_type=1, valid_bytes=72, version=[[VERSION]],
  // OFF-NEXT:    device_type=acc_device_not_host, device_number=[[OFF_DEV]],
  // OFF-NEXT:    thread_id=[[THREAD_ID]], async=acc_async_sync, async_queue=[[ASYNC_QUEUE]],
  // OFF-NEXT:    src_file=[[SRC_FILE]], func_name=main,
  // OFF-NEXT:    line_no=[[ARR0_LINE_NO]], end_line_no=[[ARR0_END_LINE_NO]],
  // OFF-NEXT:    func_line_no=[[FUNC_LINE_NO]], func_end_line_no=[[FUNC_END_LINE_NO]]
  // OFF-NEXT:  acc_other_event_info
  // OFF-NEXT:    event_type=1, valid_bytes=24,
  // OFF-NEXT:    parent_construct=acc_construct_[[ARR_CONSTRUCT]],
  // OFF-NEXT:    implicit=0, tool_info=(nil)
  // OFF-NEXT:  acc_api_info
  // OFF-NEXT:    device_api=0, valid_bytes=12,
  // OFF-NEXT:    device_type=acc_device_not_host
  // OFF-NEXT:acc_ev_device_init_end
  // OFF-NEXT:  acc_prof_info
  // OFF-NEXT:    event_type=2, valid_bytes=72, version=[[VERSION]],
  // OFF-NEXT:    device_type=acc_device_not_host, device_number=[[OFF_DEV]],
  // OFF-NEXT:    thread_id=[[THREAD_ID]], async=acc_async_sync, async_queue=[[ASYNC_QUEUE]],
  // OFF-NEXT:    src_file=[[SRC_FILE]], func_name=main,
  // OFF-NEXT:    line_no=[[ARR0_LINE_NO]], end_line_no=[[ARR0_END_LINE_NO]],
  // OFF-NEXT:    func_line_no=[[FUNC_LINE_NO]], func_end_line_no=[[FUNC_END_LINE_NO]]
  // OFF-NEXT:  acc_other_event_info
  // OFF-NEXT:    event_type=2, valid_bytes=24,
  // OFF-NEXT:    parent_construct=acc_construct_[[ARR_CONSTRUCT]],
  // OFF-NEXT:    implicit=0, tool_info=(nil)
  // OFF-NEXT:  acc_api_info
  // OFF-NEXT:    device_api=0, valid_bytes=12,
  // OFF-NEXT:    device_type=acc_device_not_host

  // Parallel construct start if -DDIR=DIR_PAR.
  //
  // CHECK-PAR-NEXT:acc_ev_compute_construct_start
  // CHECK-PAR-NEXT:  acc_prof_info
  // CHECK-PAR-NEXT:    event_type=16, valid_bytes=72, version=[[VERSION]],
  //  HOST-PAR-NEXT:    device_type=acc_device_host, device_number=[[HOST_DEV]],
  //   OFF-PAR-NEXT:    device_type=acc_device_not_host, device_number=[[OFF_DEV]],
  // CHECK-PAR-NEXT:    thread_id=[[THREAD_ID]], async=acc_async_sync, async_queue=[[ASYNC_QUEUE]],
  // CHECK-PAR-NEXT:    src_file=[[SRC_FILE]], func_name=main,
  // CHECK-PAR-NEXT:    line_no=[[KERN_LINE_NO]], end_line_no=[[KERN_END_LINE_NO]],
  // CHECK-PAR-NEXT:    func_line_no=[[FUNC_LINE_NO]], func_end_line_no=[[FUNC_END_LINE_NO]]
  // CHECK-PAR-NEXT:  acc_other_event_info
  // CHECK-PAR-NEXT:    event_type=16, valid_bytes=24,
  // CHECK-PAR-NEXT:    parent_construct=acc_construct_parallel,
  // CHECK-PAR-NEXT:    implicit=0, tool_info=(nil)
  // CHECK-PAR-NEXT:  acc_api_info
  // CHECK-PAR-NEXT:    device_api=0, valid_bytes=12,
  //  HOST-PAR-NEXT:    device_type=acc_device_host
  //   OFF-PAR-NEXT:    device_type=acc_device_not_host

  // Enter data for arr0.
  //
  //         OFF-NEXT:acc_ev_enter_data_start
  //         OFF-NEXT:  acc_prof_info
  //         OFF-NEXT:    event_type=10, valid_bytes=72, version=[[VERSION]],
  //         OFF-NEXT:    device_type=acc_device_not_host, device_number=[[OFF_DEV]],
  //         OFF-NEXT:    thread_id=[[THREAD_ID]], async=acc_async_sync, async_queue=[[ASYNC_QUEUE]],
  //         OFF-NEXT:    src_file=[[SRC_FILE]], func_name=main,
  //         OFF-NEXT:    line_no=[[ARR0_LINE_NO]], end_line_no=[[ARR0_END_LINE_NO]],
  //         OFF-NEXT:    func_line_no=[[FUNC_LINE_NO]], func_end_line_no=[[FUNC_END_LINE_NO]]
  //         OFF-NEXT:  acc_other_event_info
  //         OFF-NEXT:    event_type=10, valid_bytes=24,
  //         OFF-NEXT:    parent_construct=acc_construct_[[ARR_CONSTRUCT]],
  //         OFF-NEXT:    implicit=0, tool_info=(nil)
  //         OFF-NEXT:  acc_api_info
  //         OFF-NEXT:    device_api=0, valid_bytes=12,
  //         OFF-NEXT:    device_type=acc_device_not_host
  //         OFF-NEXT:acc_ev_alloc
  //         OFF-NEXT:  acc_prof_info
  //         OFF-NEXT:    event_type=8, valid_bytes=72, version=[[VERSION]],
  //         OFF-NEXT:    device_type=acc_device_not_host, device_number=[[OFF_DEV]],
  //         OFF-NEXT:    thread_id=[[THREAD_ID]], async=acc_async_sync, async_queue=[[ASYNC_QUEUE]],
  //         OFF-NEXT:    src_file=[[SRC_FILE]], func_name=main,
  //         OFF-NEXT:    line_no=[[ARR0_LINE_NO]], end_line_no=[[ARR0_END_LINE_NO]],
  //         OFF-NEXT:    func_line_no=[[FUNC_LINE_NO]], func_end_line_no=[[FUNC_END_LINE_NO]]
  //         OFF-NEXT:  acc_data_event_info
  //         OFF-NEXT:    event_type=8, valid_bytes=56,
  //         OFF-NEXT:    parent_construct=acc_construct_[[ARR_CONSTRUCT]],
  //         OFF-NEXT:    implicit=0, tool_info=(nil),
  //         OFF-NEXT:    var_name=(null), bytes=8,
  //         OFF-NEXT:    host_ptr=[[ARR0_HOST_PTR]],
  //         OFF-NEXT:    device_ptr=[[ARR0_DEVICE_PTR:0x[a-z0-9]+]]
  //         OFF-NEXT:  acc_api_info
  //         OFF-NEXT:    device_api=0, valid_bytes=12,
  //         OFF-NEXT:    device_type=acc_device_not_host
  //         OFF-NEXT:acc_ev_create
  //         OFF-NEXT:  acc_prof_info
  //         OFF-NEXT:    event_type=6, valid_bytes=72, version=[[VERSION]],
  //         OFF-NEXT:    device_type=acc_device_not_host, device_number=[[OFF_DEV]],
  //         OFF-NEXT:    thread_id=[[THREAD_ID]], async=acc_async_sync, async_queue=[[ASYNC_QUEUE]],
  //         OFF-NEXT:    src_file=[[SRC_FILE]], func_name=main,
  //         OFF-NEXT:    line_no=[[ARR0_LINE_NO]], end_line_no=[[ARR0_END_LINE_NO]],
  //         OFF-NEXT:    func_line_no=[[FUNC_LINE_NO]], func_end_line_no=[[FUNC_END_LINE_NO]]
  //         OFF-NEXT:  acc_data_event_info
  //         OFF-NEXT:    event_type=6, valid_bytes=56,
  //         OFF-NEXT:    parent_construct=acc_construct_[[ARR_CONSTRUCT]],
  //         OFF-NEXT:    implicit=0, tool_info=(nil),
  //         OFF-NEXT:    var_name=(null), bytes=8,
  //         OFF-NEXT:    host_ptr=[[ARR0_HOST_PTR]],
  //         OFF-NEXT:    device_ptr=[[ARR0_DEVICE_PTR]]
  //         OFF-NEXT:  acc_api_info
  //         OFF-NEXT:    device_api=0, valid_bytes=12,
  //         OFF-NEXT:    device_type=acc_device_not_host
  //         OFF-NEXT:acc_ev_enqueue_upload_start
  //         OFF-NEXT:  acc_prof_info
  //         OFF-NEXT:    event_type=20, valid_bytes=72, version=[[VERSION]],
  //         OFF-NEXT:    device_type=acc_device_not_host, device_number=[[OFF_DEV]],
  //         OFF-NEXT:    thread_id=[[THREAD_ID]], async=acc_async_sync, async_queue=[[ASYNC_QUEUE]],
  //         OFF-NEXT:    src_file=[[SRC_FILE]], func_name=main,
  //         OFF-NEXT:    line_no=[[ARR0_LINE_NO]], end_line_no=[[ARR0_END_LINE_NO]],
  //         OFF-NEXT:    func_line_no=[[FUNC_LINE_NO]], func_end_line_no=[[FUNC_END_LINE_NO]]
  //         OFF-NEXT:  acc_data_event_info
  //         OFF-NEXT:    event_type=20, valid_bytes=56,
  //         OFF-NEXT:    parent_construct=acc_construct_[[ARR_CONSTRUCT]],
  //         OFF-NEXT:    implicit=0, tool_info=(nil),
  //         OFF-NEXT:    var_name=(null), bytes=8,
  //         OFF-NEXT:    host_ptr=[[ARR0_HOST_PTR]],
  //         OFF-NEXT:    device_ptr=[[ARR0_DEVICE_PTR]]
  //         OFF-NEXT:  acc_api_info
  //         OFF-NEXT:    device_api=0, valid_bytes=12,
  //         OFF-NEXT:    device_type=acc_device_not_host
  //         OFF-NEXT:acc_ev_enqueue_upload_end
  //         OFF-NEXT:  acc_prof_info
  //         OFF-NEXT:    event_type=21, valid_bytes=72, version=[[VERSION]],
  //         OFF-NEXT:    device_type=acc_device_not_host, device_number=[[OFF_DEV]],
  //         OFF-NEXT:    thread_id=[[THREAD_ID]], async=acc_async_sync, async_queue=[[ASYNC_QUEUE]],
  //         OFF-NEXT:    src_file=[[SRC_FILE]], func_name=main,
  //         OFF-NEXT:    line_no=[[ARR0_LINE_NO]], end_line_no=[[ARR0_END_LINE_NO]],
  //         OFF-NEXT:    func_line_no=[[FUNC_LINE_NO]], func_end_line_no=[[FUNC_END_LINE_NO]]
  //         OFF-NEXT:  acc_data_event_info
  //         OFF-NEXT:    event_type=21, valid_bytes=56,
  //         OFF-NEXT:    parent_construct=acc_construct_[[ARR_CONSTRUCT]],
  //         OFF-NEXT:    implicit=0, tool_info=(nil),
  //         OFF-NEXT:    var_name=(null), bytes=8,
  //         OFF-NEXT:    host_ptr=[[ARR0_HOST_PTR]],
  //         OFF-NEXT:    device_ptr=[[ARR0_DEVICE_PTR]]
  //         OFF-NEXT:  acc_api_info
  //         OFF-NEXT:    device_api=0, valid_bytes=12,
  //         OFF-NEXT:    device_type=acc_device_not_host
  // OFF-DATAPAR-NEXT:acc_ev_enter_data_end
  // OFF-DATAPAR-NEXT:  acc_prof_info
  // OFF-DATAPAR-NEXT:    event_type=11, valid_bytes=72, version=[[VERSION]],
  // OFF-DATAPAR-NEXT:    device_type=acc_device_not_host, device_number=[[OFF_DEV]],
  // OFF-DATAPAR-NEXT:    thread_id=[[THREAD_ID]], async=acc_async_sync, async_queue=[[ASYNC_QUEUE]],
  // OFF-DATAPAR-NEXT:    src_file=[[SRC_FILE]], func_name=main,
  // OFF-DATAPAR-NEXT:    line_no=[[ARR0_LINE_NO]], end_line_no=[[ARR0_END_LINE_NO]],
  // OFF-DATAPAR-NEXT:    func_line_no=[[FUNC_LINE_NO]], func_end_line_no=[[FUNC_END_LINE_NO]]
  // OFF-DATAPAR-NEXT:  acc_other_event_info
  // OFF-DATAPAR-NEXT:    event_type=11, valid_bytes=24,
  // OFF-DATAPAR-NEXT:    parent_construct=acc_construct_data,
  // OFF-DATAPAR-NEXT:    implicit=0, tool_info=(nil)
  // OFF-DATAPAR-NEXT:  acc_api_info
  // OFF-DATAPAR-NEXT:    device_api=0, valid_bytes=12,
  // OFF-DATAPAR-NEXT:    device_type=acc_device_not_host

  // Enter data for arr1.
  //
  // OFF-DATAPAR-NEXT:acc_ev_enter_data_start
  // OFF-DATAPAR-NEXT:  acc_prof_info
  // OFF-DATAPAR-NEXT:    event_type=10, valid_bytes=72, version=[[VERSION]],
  // OFF-DATAPAR-NEXT:    device_type=acc_device_not_host, device_number=[[OFF_DEV]],
  // OFF-DATAPAR-NEXT:    thread_id=[[THREAD_ID]], async=acc_async_sync, async_queue=[[ASYNC_QUEUE]],
  // OFF-DATAPAR-NEXT:    src_file=[[SRC_FILE]], func_name=main,
  // OFF-DATAPAR-NEXT:    line_no=[[ARR1_LINE_NO]], end_line_no=[[ARR1_END_LINE_NO]],
  // OFF-DATAPAR-NEXT:    func_line_no=[[FUNC_LINE_NO]], func_end_line_no=[[FUNC_END_LINE_NO]]
  // OFF-DATAPAR-NEXT:  acc_other_event_info
  // OFF-DATAPAR-NEXT:    event_type=10, valid_bytes=24,
  // OFF-DATAPAR-NEXT:    parent_construct=acc_construct_data,
  // OFF-DATAPAR-NEXT:    implicit=0, tool_info=(nil)
  // OFF-DATAPAR-NEXT:  acc_api_info
  // OFF-DATAPAR-NEXT:    device_api=0, valid_bytes=12,
  // OFF-DATAPAR-NEXT:    device_type=acc_device_not_host
  //         OFF-NEXT:acc_ev_alloc
  //         OFF-NEXT:  acc_prof_info
  //         OFF-NEXT:    event_type=8, valid_bytes=72, version=[[VERSION]],
  //         OFF-NEXT:    device_type=acc_device_not_host, device_number=[[OFF_DEV]],
  //         OFF-NEXT:    thread_id=[[THREAD_ID]], async=acc_async_sync, async_queue=[[ASYNC_QUEUE]],
  //         OFF-NEXT:    src_file=[[SRC_FILE]], func_name=main,
  //         OFF-NEXT:    line_no=[[ARR1_LINE_NO]], end_line_no=[[ARR1_END_LINE_NO]],
  //         OFF-NEXT:    func_line_no=[[FUNC_LINE_NO]], func_end_line_no=[[FUNC_END_LINE_NO]]
  //         OFF-NEXT:  acc_data_event_info
  //         OFF-NEXT:    event_type=8, valid_bytes=56,
  //         OFF-NEXT:    parent_construct=acc_construct_[[ARR_CONSTRUCT]],
  //         OFF-NEXT:    implicit=0, tool_info=(nil),
  //         OFF-NEXT:    var_name=(null), bytes=20,
  //         OFF-NEXT:    host_ptr=[[ARR1_HOST_PTR]],
  //         OFF-NEXT:    device_ptr=[[ARR1_DEVICE_PTR:0x[a-z0-9]+]]
  //         OFF-NEXT:  acc_api_info
  //         OFF-NEXT:    device_api=0, valid_bytes=12,
  //         OFF-NEXT:    device_type=acc_device_not_host
  //         OFF-NEXT:acc_ev_create
  //         OFF-NEXT:  acc_prof_info
  //         OFF-NEXT:    event_type=6, valid_bytes=72, version=[[VERSION]],
  //         OFF-NEXT:    device_type=acc_device_not_host, device_number=[[OFF_DEV]],
  //         OFF-NEXT:    thread_id=[[THREAD_ID]], async=acc_async_sync, async_queue=[[ASYNC_QUEUE]],
  //         OFF-NEXT:    src_file=[[SRC_FILE]], func_name=main,
  //         OFF-NEXT:    line_no=[[ARR1_LINE_NO]], end_line_no=[[ARR1_END_LINE_NO]],
  //         OFF-NEXT:    func_line_no=[[FUNC_LINE_NO]], func_end_line_no=[[FUNC_END_LINE_NO]]
  //         OFF-NEXT:  acc_data_event_info
  //         OFF-NEXT:    event_type=6, valid_bytes=56,
  //         OFF-NEXT:    parent_construct=acc_construct_[[ARR_CONSTRUCT]],
  //         OFF-NEXT:    implicit=0, tool_info=(nil),
  //         OFF-NEXT:    var_name=(null), bytes=20,
  //         OFF-NEXT:    host_ptr=[[ARR1_HOST_PTR]],
  //         OFF-NEXT:    device_ptr=[[ARR1_DEVICE_PTR]]
  //         OFF-NEXT:  acc_api_info
  //         OFF-NEXT:    device_api=0, valid_bytes=12,
  //         OFF-NEXT:    device_type=acc_device_not_host
  //         OFF-NEXT:acc_ev_enqueue_upload_start
  //         OFF-NEXT:  acc_prof_info
  //         OFF-NEXT:    event_type=20, valid_bytes=72, version=[[VERSION]],
  //         OFF-NEXT:    device_type=acc_device_not_host, device_number=[[OFF_DEV]],
  //         OFF-NEXT:    thread_id=[[THREAD_ID]], async=acc_async_sync, async_queue=[[ASYNC_QUEUE]],
  //         OFF-NEXT:    src_file=[[SRC_FILE]], func_name=main,
  //         OFF-NEXT:    line_no=[[ARR1_LINE_NO]], end_line_no=[[ARR1_END_LINE_NO]],
  //         OFF-NEXT:    func_line_no=[[FUNC_LINE_NO]], func_end_line_no=[[FUNC_END_LINE_NO]]
  //         OFF-NEXT:  acc_data_event_info
  //         OFF-NEXT:    event_type=20, valid_bytes=56,
  //         OFF-NEXT:    parent_construct=acc_construct_[[ARR_CONSTRUCT]],
  //         OFF-NEXT:    implicit=0, tool_info=(nil),
  //         OFF-NEXT:    var_name=(null), bytes=20,
  //         OFF-NEXT:    host_ptr=[[ARR1_HOST_PTR]],
  //         OFF-NEXT:    device_ptr=[[ARR1_DEVICE_PTR]]
  //         OFF-NEXT:  acc_api_info
  //         OFF-NEXT:    device_api=0, valid_bytes=12,
  //         OFF-NEXT:    device_type=acc_device_not_host
  //         OFF-NEXT:acc_ev_enqueue_upload_end
  //         OFF-NEXT:  acc_prof_info
  //         OFF-NEXT:    event_type=21, valid_bytes=72, version=[[VERSION]],
  //         OFF-NEXT:    device_type=acc_device_not_host, device_number=[[OFF_DEV]],
  //         OFF-NEXT:    thread_id=[[THREAD_ID]], async=acc_async_sync, async_queue=[[ASYNC_QUEUE]],
  //         OFF-NEXT:    src_file=[[SRC_FILE]], func_name=main,
  //         OFF-NEXT:    line_no=[[ARR1_LINE_NO]], end_line_no=[[ARR1_END_LINE_NO]],
  //         OFF-NEXT:    func_line_no=[[FUNC_LINE_NO]], func_end_line_no=[[FUNC_END_LINE_NO]]
  //         OFF-NEXT:  acc_data_event_info
  //         OFF-NEXT:    event_type=21, valid_bytes=56,
  //         OFF-NEXT:    parent_construct=acc_construct_[[ARR_CONSTRUCT]],
  //         OFF-NEXT:    implicit=0, tool_info=(nil),
  //         OFF-NEXT:    var_name=(null), bytes=20,
  //         OFF-NEXT:    host_ptr=[[ARR1_HOST_PTR]],
  //         OFF-NEXT:    device_ptr=[[ARR1_DEVICE_PTR]]
  //         OFF-NEXT:  acc_api_info
  //         OFF-NEXT:    device_api=0, valid_bytes=12,
  //         OFF-NEXT:    device_type=acc_device_not_host
  //         OFF-NEXT:acc_ev_enter_data_end
  //         OFF-NEXT:  acc_prof_info
  //         OFF-NEXT:    event_type=11, valid_bytes=72, version=[[VERSION]],
  //         OFF-NEXT:    device_type=acc_device_not_host, device_number=[[OFF_DEV]],
  //         OFF-NEXT:    thread_id=[[THREAD_ID]], async=acc_async_sync, async_queue=[[ASYNC_QUEUE]],
  //         OFF-NEXT:    src_file=[[SRC_FILE]], func_name=main,
  //         OFF-NEXT:    line_no=[[ARR1_LINE_NO]], end_line_no=[[ARR1_END_LINE_NO]],
  //         OFF-NEXT:    func_line_no=[[FUNC_LINE_NO]], func_end_line_no=[[FUNC_END_LINE_NO]]
  //         OFF-NEXT:  acc_other_event_info
  //         OFF-NEXT:    event_type=11, valid_bytes=24,
  //         OFF-NEXT:    parent_construct=acc_construct_[[ARR_CONSTRUCT]],
  //         OFF-NEXT:    implicit=0, tool_info=(nil)
  //         OFF-NEXT:  acc_api_info
  //         OFF-NEXT:    device_api=0, valid_bytes=12,
  //         OFF-NEXT:    device_type=acc_device_not_host

  // Parallel construct start and associated empty enter data if
  // -DDIR=DIR_DATAPAR.
  //
  // CHECK-DATAPAR-NEXT:acc_ev_compute_construct_start
  // CHECK-DATAPAR-NEXT:  acc_prof_info
  // CHECK-DATAPAR-NEXT:    event_type=16, valid_bytes=72, version=[[VERSION]],
  //  HOST-DATAPAR-NEXT:    device_type=acc_device_host, device_number=[[HOST_DEV]],
  //   OFF-DATAPAR-NEXT:    device_type=acc_device_not_host, device_number=[[OFF_DEV]],
  // CHECK-DATAPAR-NEXT:    thread_id=[[THREAD_ID]], async=acc_async_sync, async_queue=[[ASYNC_QUEUE]],
  // CHECK-DATAPAR-NEXT:    src_file=[[SRC_FILE]], func_name=main,
  // CHECK-DATAPAR-NEXT:    line_no=[[KERN_LINE_NO]], end_line_no=[[KERN_END_LINE_NO]],
  // CHECK-DATAPAR-NEXT:    func_line_no=[[FUNC_LINE_NO]], func_end_line_no=[[FUNC_END_LINE_NO]]
  // CHECK-DATAPAR-NEXT:  acc_other_event_info
  // CHECK-DATAPAR-NEXT:    event_type=16, valid_bytes=24,
  // CHECK-DATAPAR-NEXT:    parent_construct=acc_construct_parallel,
  // CHECK-DATAPAR-NEXT:    implicit=0, tool_info=(nil)
  // CHECK-DATAPAR-NEXT:  acc_api_info
  // CHECK-DATAPAR-NEXT:    device_api=0, valid_bytes=12,
  //  HOST-DATAPAR-NEXT:    device_type=acc_device_host
  //   OFF-DATAPAR-NEXT:    device_type=acc_device_not_host
  //   OFF-DATAPAR-NEXT:acc_ev_enter_data_start
  //   OFF-DATAPAR-NEXT:  acc_prof_info
  //   OFF-DATAPAR-NEXT:    event_type=10, valid_bytes=72, version=[[VERSION]],
  //   OFF-DATAPAR-NEXT:    device_type=acc_device_not_host, device_number=[[OFF_DEV]],
  //   OFF-DATAPAR-NEXT:    thread_id=[[THREAD_ID]], async=acc_async_sync, async_queue=[[ASYNC_QUEUE]],
  //   OFF-DATAPAR-NEXT:    src_file=[[SRC_FILE]], func_name=main,
  //   OFF-DATAPAR-NEXT:    line_no=[[KERN_LINE_NO]], end_line_no=[[KERN_END_LINE_NO]],
  //   OFF-DATAPAR-NEXT:    func_line_no=[[FUNC_LINE_NO]], func_end_line_no=[[FUNC_END_LINE_NO]]
  //   OFF-DATAPAR-NEXT:  acc_other_event_info
  //   OFF-DATAPAR-NEXT:    event_type=10, valid_bytes=24,
  //   OFF-DATAPAR-NEXT:    parent_construct=acc_construct_parallel,
  //   OFF-DATAPAR-NEXT:    implicit=0, tool_info=(nil)
  //   OFF-DATAPAR-NEXT:  acc_api_info
  //   OFF-DATAPAR-NEXT:    device_api=0, valid_bytes=12,
  //   OFF-DATAPAR-NEXT:    device_type=acc_device_not_host
  //   OFF-DATAPAR-NEXT:acc_ev_enter_data_end
  //   OFF-DATAPAR-NEXT:  acc_prof_info
  //   OFF-DATAPAR-NEXT:    event_type=11, valid_bytes=72, version=[[VERSION]],
  //   OFF-DATAPAR-NEXT:    device_type=acc_device_not_host, device_number=[[OFF_DEV]],
  //   OFF-DATAPAR-NEXT:    thread_id=[[THREAD_ID]], async=acc_async_sync, async_queue=[[ASYNC_QUEUE]],
  //   OFF-DATAPAR-NEXT:    src_file=[[SRC_FILE]], func_name=main,
  //   OFF-DATAPAR-NEXT:    line_no=[[KERN_LINE_NO]], end_line_no=[[KERN_END_LINE_NO]],
  //   OFF-DATAPAR-NEXT:    func_line_no=[[FUNC_LINE_NO]], func_end_line_no=[[FUNC_END_LINE_NO]]
  //   OFF-DATAPAR-NEXT:  acc_other_event_info
  //   OFF-DATAPAR-NEXT:    event_type=11, valid_bytes=24,
  //   OFF-DATAPAR-NEXT:    parent_construct=acc_construct_parallel,
  //   OFF-DATAPAR-NEXT:    implicit=0, tool_info=(nil)
  //   OFF-DATAPAR-NEXT:  acc_api_info
  //   OFF-DATAPAR-NEXT:    device_api=0, valid_bytes=12,
  //   OFF-DATAPAR-NEXT:    device_type=acc_device_not_host

  // Enqueue launch.
  //
  // CHECK-HASPAR-NEXT:acc_ev_enqueue_launch_start
  // CHECK-HASPAR-NEXT:  acc_prof_info
  // CHECK-HASPAR-NEXT:    event_type=18, valid_bytes=72, version=[[VERSION]],
  //  HOST-HASPAR-NEXT:    device_type=acc_device_host, device_number=[[HOST_DEV]],
  //   OFF-HASPAR-NEXT:    device_type=acc_device_not_host, device_number=[[OFF_DEV]],
  // CHECK-HASPAR-NEXT:    thread_id=[[THREAD_ID]], async=acc_async_sync, async_queue=[[ASYNC_QUEUE]],
  // CHECK-HASPAR-NEXT:    src_file=[[SRC_FILE]], func_name=main,
  // CHECK-HASPAR-NEXT:    line_no=[[KERN_LINE_NO]], end_line_no=[[KERN_END_LINE_NO]],
  // CHECK-HASPAR-NEXT:    func_line_no=[[FUNC_LINE_NO]], func_end_line_no=[[FUNC_END_LINE_NO]]
  // CHECK-HASPAR-NEXT:  acc_launch_event_info
  // CHECK-HASPAR-NEXT:    event_type=18, valid_bytes=32,
  // CHECK-HASPAR-NEXT:    parent_construct=acc_construct_parallel,
  // CHECK-HASPAR-NEXT:    implicit=0, tool_info=(nil),
  // CHECK-HASPAR-NEXT:    kernel_name=(nil)
  // CHECK-HASPAR-NEXT:  acc_api_info
  // CHECK-HASPAR-NEXT:    device_api=0, valid_bytes=12,
  //  HOST-HASPAR-NEXT:    device_type=acc_device_host
  //   OFF-HASPAR-NEXT:    device_type=acc_device_not_host
  // CHECK-HASPAR-NEXT:acc_ev_enqueue_launch_end
  // CHECK-HASPAR-NEXT:  acc_prof_info
  // CHECK-HASPAR-NEXT:    event_type=19, valid_bytes=72, version=[[VERSION]],
  //  HOST-HASPAR-NEXT:    device_type=acc_device_host, device_number=[[HOST_DEV]],
  //   OFF-HASPAR-NEXT:    device_type=acc_device_not_host, device_number=[[OFF_DEV]],
  // CHECK-HASPAR-NEXT:    thread_id=[[THREAD_ID]], async=acc_async_sync, async_queue=[[ASYNC_QUEUE]],
  // CHECK-HASPAR-NEXT:    src_file=[[SRC_FILE]], func_name=main,
  // CHECK-HASPAR-NEXT:    line_no=[[KERN_LINE_NO]], end_line_no=[[KERN_END_LINE_NO]],
  // CHECK-HASPAR-NEXT:    func_line_no=[[FUNC_LINE_NO]], func_end_line_no=[[FUNC_END_LINE_NO]]
  // CHECK-HASPAR-NEXT:  acc_launch_event_info
  // CHECK-HASPAR-NEXT:    event_type=19, valid_bytes=32,
  // CHECK-HASPAR-NEXT:    parent_construct=acc_construct_parallel,
  // CHECK-HASPAR-NEXT:    implicit=0, tool_info=(nil),
  // CHECK-HASPAR-NEXT:    kernel_name=(nil)
  // CHECK-HASPAR-NEXT:  acc_api_info
  // CHECK-HASPAR-NEXT:    device_api=0, valid_bytes=12,
  //  HOST-HASPAR-NEXT:    device_type=acc_device_host
  //   OFF-HASPAR-NEXT:    device_type=acc_device_not_host

#if DIR == DIR_DATA
  #line 20000
  #pragma acc data copy(arr0, arr1)
  #line 30000
  ;
#elif DIR == DIR_PAR
  #line 40000
  #pragma acc parallel copy(arr0, arr1) num_gangs(1)
#elif DIR == DIR_DATAPAR
  #line 50000
  #pragma acc data copy(arr0)
  {
    #line 60000
    #pragma acc data copy(arr1)
    {
      #line 70000
      #pragma acc parallel num_gangs(1)
#else
# error undefined DIR
#endif
#if DIR == DIR_PAR || DIR == DIR_DATAPAR
      for (int j = 0; j < 2; ++j) {
        //    HOST-HASPAR-NEXT:inside: arr0=[[ARR0_HOST_PTR]], arr0[0]=10
        //    HOST-HASPAR-NEXT:inside: arr1=[[ARR1_HOST_PTR]], arr1[0]=20
        //    HOST-HASPAR-NEXT:inside: arr0=[[ARR0_HOST_PTR]], arr0[1]=11
        //    HOST-HASPAR-NEXT:inside: arr1=[[ARR1_HOST_PTR]], arr1[1]=21
        //  X86_64-HASPAR-NEXT:inside: arr0=[[ARR0_DEVICE_PTR]], arr0[0]=10
        //  X86_64-HASPAR-NEXT:inside: arr1=[[ARR1_DEVICE_PTR]], arr1[0]=20
        //  X86_64-HASPAR-NEXT:inside: arr0=[[ARR0_DEVICE_PTR]], arr0[1]=11
        //  X86_64-HASPAR-NEXT:inside: arr1=[[ARR1_DEVICE_PTR]], arr1[1]=21
        // PPC64LE-HASPAR-NEXT:inside: arr0=[[ARR0_DEVICE_PTR]], arr0[0]=10
        // PPC64LE-HASPAR-NEXT:inside: arr1=[[ARR1_DEVICE_PTR]], arr1[0]=20
        // PPC64LE-HASPAR-NEXT:inside: arr0=[[ARR0_DEVICE_PTR]], arr0[1]=11
        // PPC64LE-HASPAR-NEXT:inside: arr1=[[ARR1_DEVICE_PTR]], arr1[1]=21
        // We omit NVPTX64 here because subsequent events might trigger before
        // kernel execution due to the use of CUDA streams.
        if (!NVPTX64 || offloadDisabled) {
          printf("inside: arr0=%p, arr0[%d]=%d\n", arr0, j, arr0[j]);
          printf("inside: arr1=%p, arr1[%d]=%d\n", arr1, j, arr1[j]);
        }
      #line 80000
      }
#endif
#if DIR == DIR_DATAPAR
    #line 90000
    #pragma acc update self(arr0, notPresent0) device(arr1, notPresent1)
    #line 100000
    #pragma acc update self(notPresent0) device(notPresent1)
    #line 110000
    }
  #line 120000
  }
#endif

  // Parallel construct end and associated empty exit data if
  // -DDIR=DIR_DATAPAR.
  //
  //   OFF-DATAPAR-NEXT:acc_ev_exit_data_start
  //   OFF-DATAPAR-NEXT:  acc_prof_info
  //   OFF-DATAPAR-NEXT:    event_type=12, valid_bytes=72, version=[[VERSION]],
  //   OFF-DATAPAR-NEXT:    device_type=acc_device_not_host, device_number=[[OFF_DEV]],
  //   OFF-DATAPAR-NEXT:    thread_id=[[THREAD_ID]], async=acc_async_sync, async_queue=[[ASYNC_QUEUE]],
  //   OFF-DATAPAR-NEXT:    src_file=[[SRC_FILE]], func_name=main,
  //   OFF-DATAPAR-NEXT:    line_no=[[KERN_LINE_NO]], end_line_no=[[KERN_END_LINE_NO]],
  //   OFF-DATAPAR-NEXT:    func_line_no=[[FUNC_LINE_NO]], func_end_line_no=[[FUNC_END_LINE_NO]]
  //   OFF-DATAPAR-NEXT:  acc_other_event_info
  //   OFF-DATAPAR-NEXT:    event_type=12, valid_bytes=24,
  //   OFF-DATAPAR-NEXT:    parent_construct=acc_construct_parallel,
  //   OFF-DATAPAR-NEXT:    implicit=0, tool_info=(nil)
  //   OFF-DATAPAR-NEXT:  acc_api_info
  //   OFF-DATAPAR-NEXT:    device_api=0, valid_bytes=12,
  //   OFF-DATAPAR-NEXT:    device_type=acc_device_not_host
  //   OFF-DATAPAR-NEXT:acc_ev_exit_data_end
  //   OFF-DATAPAR-NEXT:  acc_prof_info
  //   OFF-DATAPAR-NEXT:    event_type=13, valid_bytes=72, version=[[VERSION]],
  //   OFF-DATAPAR-NEXT:    device_type=acc_device_not_host, device_number=[[OFF_DEV]],
  //   OFF-DATAPAR-NEXT:    thread_id=[[THREAD_ID]], async=acc_async_sync, async_queue=[[ASYNC_QUEUE]],
  //   OFF-DATAPAR-NEXT:    src_file=[[SRC_FILE]], func_name=main,
  //   OFF-DATAPAR-NEXT:    line_no=[[KERN_LINE_NO]], end_line_no=[[KERN_END_LINE_NO]],
  //   OFF-DATAPAR-NEXT:    func_line_no=[[FUNC_LINE_NO]], func_end_line_no=[[FUNC_END_LINE_NO]]
  //   OFF-DATAPAR-NEXT:  acc_other_event_info
  //   OFF-DATAPAR-NEXT:    event_type=13, valid_bytes=24,
  //   OFF-DATAPAR-NEXT:    parent_construct=acc_construct_parallel,
  //   OFF-DATAPAR-NEXT:    implicit=0, tool_info=(nil)
  //   OFF-DATAPAR-NEXT:  acc_api_info
  //   OFF-DATAPAR-NEXT:    device_api=0, valid_bytes=12,
  //   OFF-DATAPAR-NEXT:    device_type=acc_device_not_host
  // CHECK-DATAPAR-NEXT:acc_ev_compute_construct_end
  // CHECK-DATAPAR-NEXT:  acc_prof_info
  // CHECK-DATAPAR-NEXT:    event_type=17, valid_bytes=72, version=[[VERSION]],
  //  HOST-DATAPAR-NEXT:    device_type=acc_device_host, device_number=[[HOST_DEV]],
  //   OFF-DATAPAR-NEXT:    device_type=acc_device_not_host, device_number=[[OFF_DEV]],
  // CHECK-DATAPAR-NEXT:    thread_id=[[THREAD_ID]], async=acc_async_sync, async_queue=[[ASYNC_QUEUE]],
  // CHECK-DATAPAR-NEXT:    src_file=[[SRC_FILE]], func_name=main,
  // CHECK-DATAPAR-NEXT:    line_no=[[KERN_LINE_NO]], end_line_no=[[KERN_END_LINE_NO]],
  // CHECK-DATAPAR-NEXT:    func_line_no=[[FUNC_LINE_NO]], func_end_line_no=[[FUNC_END_LINE_NO]]
  // CHECK-DATAPAR-NEXT:  acc_other_event_info
  // CHECK-DATAPAR-NEXT:    event_type=17, valid_bytes=24,
  // CHECK-DATAPAR-NEXT:    parent_construct=acc_construct_parallel,
  // CHECK-DATAPAR-NEXT:    implicit=0, tool_info=(nil)
  // CHECK-DATAPAR-NEXT:  acc_api_info
  // CHECK-DATAPAR-NEXT:    device_api=0, valid_bytes=12,
  //  HOST-DATAPAR-NEXT:    device_type=acc_device_host
  //   OFF-DATAPAR-NEXT:    device_type=acc_device_not_host

  // Update directive events if -DDIR=DIR_DATAPAR.
  //
  // OFF-DATAPAR-NEXT:acc_ev_update_start
  // OFF-DATAPAR-NEXT:  acc_prof_info
  // OFF-DATAPAR-NEXT:    event_type=14, valid_bytes=72, version=[[VERSION]],
  // OFF-DATAPAR-NEXT:    device_type=acc_device_not_host, device_number=[[OFF_DEV]],
  // OFF-DATAPAR-NEXT:    thread_id=[[THREAD_ID]], async=acc_async_sync, async_queue=[[ASYNC_QUEUE]],
  // OFF-DATAPAR-NEXT:    src_file=[[SRC_FILE]], func_name=main,
  // OFF-DATAPAR-NEXT:    line_no=[[UPDATE0_LINE_NO]], end_line_no=[[UPDATE0_LINE_NO]],
  // OFF-DATAPAR-NEXT:    func_line_no=[[FUNC_LINE_NO]], func_end_line_no=[[FUNC_END_LINE_NO]]
  // OFF-DATAPAR-NEXT:  acc_other_event_info
  // OFF-DATAPAR-NEXT:    event_type=14, valid_bytes=24,
  // OFF-DATAPAR-NEXT:    parent_construct=acc_construct_update,
  // OFF-DATAPAR-NEXT:    implicit=0, tool_info=(nil)
  // OFF-DATAPAR-NEXT:  acc_api_info
  // OFF-DATAPAR-NEXT:    device_api=0, valid_bytes=12,
  // OFF-DATAPAR-NEXT:    device_type=acc_device_not_host
  // OFF-DATAPAR-NEXT:acc_ev_enqueue_upload_start
  // OFF-DATAPAR-NEXT:  acc_prof_info
  // OFF-DATAPAR-NEXT:    event_type=20, valid_bytes=72, version=[[VERSION]],
  // OFF-DATAPAR-NEXT:    device_type=acc_device_not_host, device_number=[[OFF_DEV]],
  // OFF-DATAPAR-NEXT:    thread_id=[[THREAD_ID]], async=acc_async_sync, async_queue=[[ASYNC_QUEUE]],
  // OFF-DATAPAR-NEXT:    src_file=[[SRC_FILE]], func_name=main,
  // OFF-DATAPAR-NEXT:    line_no=[[UPDATE0_LINE_NO]], end_line_no=[[UPDATE0_LINE_NO]],
  // OFF-DATAPAR-NEXT:    func_line_no=[[FUNC_LINE_NO]], func_end_line_no=[[FUNC_END_LINE_NO]]
  // OFF-DATAPAR-NEXT:  acc_data_event_info
  // OFF-DATAPAR-NEXT:    event_type=20, valid_bytes=56,
  // OFF-DATAPAR-NEXT:    parent_construct=acc_construct_update,
  // OFF-DATAPAR-NEXT:    implicit=0, tool_info=(nil),
  // OFF-DATAPAR-NEXT:    var_name=(null), bytes=20,
  // OFF-DATAPAR-NEXT:    host_ptr=[[ARR1_HOST_PTR]],
  // OFF-DATAPAR-NEXT:    device_ptr=[[ARR1_DEVICE_PTR]]
  // OFF-DATAPAR-NEXT:  acc_api_info
  // OFF-DATAPAR-NEXT:    device_api=0, valid_bytes=12,
  // OFF-DATAPAR-NEXT:    device_type=acc_device_not_host
  // OFF-DATAPAR-NEXT:acc_ev_enqueue_upload_end
  // OFF-DATAPAR-NEXT:  acc_prof_info
  // OFF-DATAPAR-NEXT:    event_type=21, valid_bytes=72, version=[[VERSION]],
  // OFF-DATAPAR-NEXT:    device_type=acc_device_not_host, device_number=[[OFF_DEV]],
  // OFF-DATAPAR-NEXT:    thread_id=[[THREAD_ID]], async=acc_async_sync, async_queue=[[ASYNC_QUEUE]],
  // OFF-DATAPAR-NEXT:    src_file=[[SRC_FILE]], func_name=main,
  // OFF-DATAPAR-NEXT:    line_no=[[UPDATE0_LINE_NO]], end_line_no=[[UPDATE0_LINE_NO]],
  // OFF-DATAPAR-NEXT:    func_line_no=[[FUNC_LINE_NO]], func_end_line_no=[[FUNC_END_LINE_NO]]
  // OFF-DATAPAR-NEXT:  acc_data_event_info
  // OFF-DATAPAR-NEXT:    event_type=21, valid_bytes=56,
  // OFF-DATAPAR-NEXT:    parent_construct=acc_construct_update,
  // OFF-DATAPAR-NEXT:    implicit=0, tool_info=(nil),
  // OFF-DATAPAR-NEXT:    var_name=(null), bytes=20,
  // OFF-DATAPAR-NEXT:    host_ptr=[[ARR1_HOST_PTR]],
  // OFF-DATAPAR-NEXT:    device_ptr=[[ARR1_DEVICE_PTR]]
  // OFF-DATAPAR-NEXT:  acc_api_info
  // OFF-DATAPAR-NEXT:    device_api=0, valid_bytes=12,
  // OFF-DATAPAR-NEXT:    device_type=acc_device_not_host
  // OFF-DATAPAR-NEXT:acc_ev_enqueue_download_start
  // OFF-DATAPAR-NEXT:  acc_prof_info
  // OFF-DATAPAR-NEXT:    event_type=22, valid_bytes=72, version=[[VERSION]],
  // OFF-DATAPAR-NEXT:    device_type=acc_device_not_host, device_number=[[OFF_DEV]],
  // OFF-DATAPAR-NEXT:    thread_id=[[THREAD_ID]], async=acc_async_sync, async_queue=[[ASYNC_QUEUE]],
  // OFF-DATAPAR-NEXT:    src_file=[[SRC_FILE]], func_name=main,
  // OFF-DATAPAR-NEXT:    line_no=[[UPDATE0_LINE_NO]], end_line_no=[[UPDATE0_LINE_NO]],
  // OFF-DATAPAR-NEXT:    func_line_no=[[FUNC_LINE_NO]], func_end_line_no=[[FUNC_END_LINE_NO]]
  // OFF-DATAPAR-NEXT:  acc_data_event_info
  // OFF-DATAPAR-NEXT:    event_type=22, valid_bytes=56,
  // OFF-DATAPAR-NEXT:    parent_construct=acc_construct_update,
  // OFF-DATAPAR-NEXT:    implicit=0, tool_info=(nil),
  // OFF-DATAPAR-NEXT:    var_name=(null), bytes=8,
  // OFF-DATAPAR-NEXT:    host_ptr=[[ARR0_HOST_PTR]],
  // OFF-DATAPAR-NEXT:    device_ptr=[[ARR0_DEVICE_PTR]]
  // OFF-DATAPAR-NEXT:  acc_api_info
  // OFF-DATAPAR-NEXT:    device_api=0, valid_bytes=12,
  // OFF-DATAPAR-NEXT:    device_type=acc_device_not_host
  // OFF-DATAPAR-NEXT:acc_ev_enqueue_download_end
  // OFF-DATAPAR-NEXT:  acc_prof_info
  // OFF-DATAPAR-NEXT:    event_type=23, valid_bytes=72, version=[[VERSION]],
  // OFF-DATAPAR-NEXT:    device_type=acc_device_not_host, device_number=[[OFF_DEV]],
  // OFF-DATAPAR-NEXT:    thread_id=[[THREAD_ID]], async=acc_async_sync, async_queue=[[ASYNC_QUEUE]],
  // OFF-DATAPAR-NEXT:    src_file=[[SRC_FILE]], func_name=main,
  // OFF-DATAPAR-NEXT:    line_no=[[UPDATE0_LINE_NO]], end_line_no=[[UPDATE0_LINE_NO]],
  // OFF-DATAPAR-NEXT:    func_line_no=[[FUNC_LINE_NO]], func_end_line_no=[[FUNC_END_LINE_NO]]
  // OFF-DATAPAR-NEXT:  acc_data_event_info
  // OFF-DATAPAR-NEXT:    event_type=23, valid_bytes=56,
  // OFF-DATAPAR-NEXT:    parent_construct=acc_construct_update,
  // OFF-DATAPAR-NEXT:    implicit=0, tool_info=(nil),
  // OFF-DATAPAR-NEXT:    var_name=(null), bytes=8,
  // OFF-DATAPAR-NEXT:    host_ptr=[[ARR0_HOST_PTR]],
  // OFF-DATAPAR-NEXT:    device_ptr=[[ARR0_DEVICE_PTR]]
  // OFF-DATAPAR-NEXT:  acc_api_info
  // OFF-DATAPAR-NEXT:    device_api=0, valid_bytes=12,
  // OFF-DATAPAR-NEXT:    device_type=acc_device_not_host
  // OFF-DATAPAR-NEXT:acc_ev_update_end
  // OFF-DATAPAR-NEXT:  acc_prof_info
  // OFF-DATAPAR-NEXT:    event_type=15, valid_bytes=72, version=[[VERSION]],
  // OFF-DATAPAR-NEXT:    device_type=acc_device_not_host, device_number=[[OFF_DEV]],
  // OFF-DATAPAR-NEXT:    thread_id=[[THREAD_ID]], async=acc_async_sync, async_queue=[[ASYNC_QUEUE]],
  // OFF-DATAPAR-NEXT:    src_file=[[SRC_FILE]], func_name=main,
  // OFF-DATAPAR-NEXT:    line_no=[[UPDATE0_LINE_NO]], end_line_no=[[UPDATE0_LINE_NO]],
  // OFF-DATAPAR-NEXT:    func_line_no=[[FUNC_LINE_NO]], func_end_line_no=[[FUNC_END_LINE_NO]]
  // OFF-DATAPAR-NEXT:  acc_other_event_info
  // OFF-DATAPAR-NEXT:    event_type=15, valid_bytes=24,
  // OFF-DATAPAR-NEXT:    parent_construct=acc_construct_update,
  // OFF-DATAPAR-NEXT:    implicit=0, tool_info=(nil)
  // OFF-DATAPAR-NEXT:  acc_api_info
  // OFF-DATAPAR-NEXT:    device_api=0, valid_bytes=12,
  // OFF-DATAPAR-NEXT:    device_type=acc_device_not_host
  // OFF-DATAPAR-NEXT:acc_ev_update_start
  // OFF-DATAPAR-NEXT:  acc_prof_info
  // OFF-DATAPAR-NEXT:    event_type=14, valid_bytes=72, version=[[VERSION]],
  // OFF-DATAPAR-NEXT:    device_type=acc_device_not_host, device_number=[[OFF_DEV]],
  // OFF-DATAPAR-NEXT:    thread_id=[[THREAD_ID]], async=acc_async_sync, async_queue=[[ASYNC_QUEUE]],
  // OFF-DATAPAR-NEXT:    src_file=[[SRC_FILE]], func_name=main,
  // OFF-DATAPAR-NEXT:    line_no=[[UPDATE1_LINE_NO]], end_line_no=[[UPDATE1_LINE_NO]],
  // OFF-DATAPAR-NEXT:    func_line_no=[[FUNC_LINE_NO]], func_end_line_no=[[FUNC_END_LINE_NO]]
  // OFF-DATAPAR-NEXT:  acc_other_event_info
  // OFF-DATAPAR-NEXT:    event_type=14, valid_bytes=24,
  // OFF-DATAPAR-NEXT:    parent_construct=acc_construct_update,
  // OFF-DATAPAR-NEXT:    implicit=0, tool_info=(nil)
  // OFF-DATAPAR-NEXT:  acc_api_info
  // OFF-DATAPAR-NEXT:    device_api=0, valid_bytes=12,
  // OFF-DATAPAR-NEXT:    device_type=acc_device_not_host
  // OFF-DATAPAR-NEXT:acc_ev_update_end
  // OFF-DATAPAR-NEXT:  acc_prof_info
  // OFF-DATAPAR-NEXT:    event_type=15, valid_bytes=72, version=[[VERSION]],
  // OFF-DATAPAR-NEXT:    device_type=acc_device_not_host, device_number=[[OFF_DEV]],
  // OFF-DATAPAR-NEXT:    thread_id=[[THREAD_ID]], async=acc_async_sync, async_queue=[[ASYNC_QUEUE]],
  // OFF-DATAPAR-NEXT:    src_file=[[SRC_FILE]], func_name=main,
  // OFF-DATAPAR-NEXT:    line_no=[[UPDATE1_LINE_NO]], end_line_no=[[UPDATE1_LINE_NO]],
  // OFF-DATAPAR-NEXT:    func_line_no=[[FUNC_LINE_NO]], func_end_line_no=[[FUNC_END_LINE_NO]]
  // OFF-DATAPAR-NEXT:  acc_other_event_info
  // OFF-DATAPAR-NEXT:    event_type=15, valid_bytes=24,
  // OFF-DATAPAR-NEXT:    parent_construct=acc_construct_update,
  // OFF-DATAPAR-NEXT:    implicit=0, tool_info=(nil)
  // OFF-DATAPAR-NEXT:  acc_api_info
  // OFF-DATAPAR-NEXT:    device_api=0, valid_bytes=12,
  // OFF-DATAPAR-NEXT:    device_type=acc_device_not_host

  // Exit data for arr1 and then arr0 if not DATAPAR.
  //
  // OFF-NODATAPAR-NEXT:acc_ev_exit_data_start
  // OFF-NODATAPAR-NEXT:  acc_prof_info
  // OFF-NODATAPAR-NEXT:    event_type=12, valid_bytes=72, version=[[VERSION]],
  // OFF-NODATAPAR-NEXT:    device_type=acc_device_not_host, device_number=[[OFF_DEV]],
  // OFF-NODATAPAR-NEXT:    thread_id=[[THREAD_ID]], async=acc_async_sync, async_queue=[[ASYNC_QUEUE]],
  // OFF-NODATAPAR-NEXT:    src_file=[[SRC_FILE]], func_name=main,
  // OFF-NODATAPAR-NEXT:    line_no=[[ARR1_LINE_NO]], end_line_no=[[ARR1_END_LINE_NO]],
  // OFF-NODATAPAR-NEXT:    func_line_no=[[FUNC_LINE_NO]], func_end_line_no=[[FUNC_END_LINE_NO]]
  // OFF-NODATAPAR-NEXT:  acc_other_event_info
  // OFF-NODATAPAR-NEXT:    event_type=12, valid_bytes=24,
  // OFF-NODATAPAR-NEXT:    parent_construct=acc_construct_[[ARR_CONSTRUCT]],
  // OFF-NODATAPAR-NEXT:    implicit=0, tool_info=(nil)
  // OFF-NODATAPAR-NEXT:  acc_api_info
  // OFF-NODATAPAR-NEXT:    device_api=0, valid_bytes=12,
  // OFF-NODATAPAR-NEXT:    device_type=acc_device_not_host
  // OFF-NODATAPAR-NEXT:acc_ev_enqueue_download_start
  // OFF-NODATAPAR-NEXT:  acc_prof_info
  // OFF-NODATAPAR-NEXT:    event_type=22, valid_bytes=72, version=[[VERSION]],
  // OFF-NODATAPAR-NEXT:    device_type=acc_device_not_host, device_number=[[OFF_DEV]],
  // OFF-NODATAPAR-NEXT:    thread_id=[[THREAD_ID]], async=acc_async_sync, async_queue=[[ASYNC_QUEUE]],
  // OFF-NODATAPAR-NEXT:    src_file=[[SRC_FILE]], func_name=main,
  // OFF-NODATAPAR-NEXT:    line_no=[[ARR1_LINE_NO]], end_line_no=[[ARR1_END_LINE_NO]],
  // OFF-NODATAPAR-NEXT:    func_line_no=[[FUNC_LINE_NO]], func_end_line_no=[[FUNC_END_LINE_NO]]
  // OFF-NODATAPAR-NEXT:  acc_data_event_info
  // OFF-NODATAPAR-NEXT:    event_type=22, valid_bytes=56,
  // OFF-NODATAPAR-NEXT:    parent_construct=acc_construct_[[ARR_CONSTRUCT]],
  // OFF-NODATAPAR-NEXT:    implicit=0, tool_info=(nil),
  // OFF-NODATAPAR-NEXT:    var_name=(null), bytes=20,
  // OFF-NODATAPAR-NEXT:    host_ptr=[[ARR1_HOST_PTR]],
  // OFF-NODATAPAR-NEXT:    device_ptr=[[ARR1_DEVICE_PTR]]
  // OFF-NODATAPAR-NEXT:  acc_api_info
  // OFF-NODATAPAR-NEXT:    device_api=0, valid_bytes=12,
  // OFF-NODATAPAR-NEXT:    device_type=acc_device_not_host
  // OFF-NODATAPAR-NEXT:acc_ev_enqueue_download_end
  // OFF-NODATAPAR-NEXT:  acc_prof_info
  // OFF-NODATAPAR-NEXT:    event_type=23, valid_bytes=72, version=[[VERSION]],
  // OFF-NODATAPAR-NEXT:    device_type=acc_device_not_host, device_number=[[OFF_DEV]],
  // OFF-NODATAPAR-NEXT:    thread_id=[[THREAD_ID]], async=acc_async_sync, async_queue=[[ASYNC_QUEUE]],
  // OFF-NODATAPAR-NEXT:    src_file=[[SRC_FILE]], func_name=main,
  // OFF-NODATAPAR-NEXT:    line_no=[[ARR1_LINE_NO]], end_line_no=[[ARR1_END_LINE_NO]],
  // OFF-NODATAPAR-NEXT:    func_line_no=[[FUNC_LINE_NO]], func_end_line_no=[[FUNC_END_LINE_NO]]
  // OFF-NODATAPAR-NEXT:  acc_data_event_info
  // OFF-NODATAPAR-NEXT:    event_type=23, valid_bytes=56,
  // OFF-NODATAPAR-NEXT:    parent_construct=acc_construct_[[ARR_CONSTRUCT]],
  // OFF-NODATAPAR-NEXT:    implicit=0, tool_info=(nil),
  // OFF-NODATAPAR-NEXT:    var_name=(null), bytes=20,
  // OFF-NODATAPAR-NEXT:    host_ptr=[[ARR1_HOST_PTR]],
  // OFF-NODATAPAR-NEXT:    device_ptr=[[ARR1_DEVICE_PTR]]
  // OFF-NODATAPAR-NEXT:  acc_api_info
  // OFF-NODATAPAR-NEXT:    device_api=0, valid_bytes=12,
  // OFF-NODATAPAR-NEXT:    device_type=acc_device_not_host
  // OFF-NODATAPAR-NEXT:acc_ev_enqueue_download_start
  // OFF-NODATAPAR-NEXT:  acc_prof_info
  // OFF-NODATAPAR-NEXT:    event_type=22, valid_bytes=72, version=[[VERSION]],
  // OFF-NODATAPAR-NEXT:    device_type=acc_device_not_host, device_number=[[OFF_DEV]],
  // OFF-NODATAPAR-NEXT:    thread_id=[[THREAD_ID]], async=acc_async_sync, async_queue=[[ASYNC_QUEUE]],
  // OFF-NODATAPAR-NEXT:    src_file=[[SRC_FILE]], func_name=main,
  // OFF-NODATAPAR-NEXT:    line_no=[[ARR0_LINE_NO]], end_line_no=[[ARR0_END_LINE_NO]],
  // OFF-NODATAPAR-NEXT:    func_line_no=[[FUNC_LINE_NO]], func_end_line_no=[[FUNC_END_LINE_NO]]
  // OFF-NODATAPAR-NEXT:  acc_data_event_info
  // OFF-NODATAPAR-NEXT:    event_type=22, valid_bytes=56,
  // OFF-NODATAPAR-NEXT:    parent_construct=acc_construct_[[ARR_CONSTRUCT]],
  // OFF-NODATAPAR-NEXT:    implicit=0, tool_info=(nil),
  // OFF-NODATAPAR-NEXT:    var_name=(null), bytes=8,
  // OFF-NODATAPAR-NEXT:    host_ptr=[[ARR0_HOST_PTR]],
  // OFF-NODATAPAR-NEXT:    device_ptr=[[ARR0_DEVICE_PTR]]
  // OFF-NODATAPAR-NEXT:  acc_api_info
  // OFF-NODATAPAR-NEXT:    device_api=0, valid_bytes=12,
  // OFF-NODATAPAR-NEXT:    device_type=acc_device_not_host
  // OFF-NODATAPAR-NEXT:acc_ev_enqueue_download_end
  // OFF-NODATAPAR-NEXT:  acc_prof_info
  // OFF-NODATAPAR-NEXT:    event_type=23, valid_bytes=72, version=[[VERSION]],
  // OFF-NODATAPAR-NEXT:    device_type=acc_device_not_host, device_number=[[OFF_DEV]],
  // OFF-NODATAPAR-NEXT:    thread_id=[[THREAD_ID]], async=acc_async_sync, async_queue=[[ASYNC_QUEUE]],
  // OFF-NODATAPAR-NEXT:    src_file=[[SRC_FILE]], func_name=main,
  // OFF-NODATAPAR-NEXT:    line_no=[[ARR0_LINE_NO]], end_line_no=[[ARR0_END_LINE_NO]],
  // OFF-NODATAPAR-NEXT:    func_line_no=[[FUNC_LINE_NO]], func_end_line_no=[[FUNC_END_LINE_NO]]
  // OFF-NODATAPAR-NEXT:  acc_data_event_info
  // OFF-NODATAPAR-NEXT:    event_type=23, valid_bytes=56,
  // OFF-NODATAPAR-NEXT:    parent_construct=acc_construct_[[ARR_CONSTRUCT]],
  // OFF-NODATAPAR-NEXT:    implicit=0, tool_info=(nil),
  // OFF-NODATAPAR-NEXT:    var_name=(null), bytes=8,
  // OFF-NODATAPAR-NEXT:    host_ptr=[[ARR0_HOST_PTR]],
  // OFF-NODATAPAR-NEXT:    device_ptr=[[ARR0_DEVICE_PTR]]
  // OFF-NODATAPAR-NEXT:  acc_api_info
  // OFF-NODATAPAR-NEXT:    device_api=0, valid_bytes=12,
  // OFF-NODATAPAR-NEXT:    device_type=acc_device_not_host
  // OFF-NODATAPAR-NEXT:acc_ev_delete
  // OFF-NODATAPAR-NEXT:  acc_prof_info
  // OFF-NODATAPAR-NEXT:    event_type=7, valid_bytes=72, version=[[VERSION]],
  // OFF-NODATAPAR-NEXT:    device_type=acc_device_not_host, device_number=[[OFF_DEV]],
  // OFF-NODATAPAR-NEXT:    thread_id=[[THREAD_ID]], async=acc_async_sync, async_queue=[[ASYNC_QUEUE]],
  // OFF-NODATAPAR-NEXT:    src_file=[[SRC_FILE]], func_name=main,
  // OFF-NODATAPAR-NEXT:    line_no=[[ARR1_LINE_NO]], end_line_no=[[ARR1_END_LINE_NO]],
  // OFF-NODATAPAR-NEXT:    func_line_no=[[FUNC_LINE_NO]], func_end_line_no=[[FUNC_END_LINE_NO]]
  // OFF-NODATAPAR-NEXT:  acc_data_event_info
  // OFF-NODATAPAR-NEXT:    event_type=7, valid_bytes=56,
  // OFF-NODATAPAR-NEXT:    parent_construct=acc_construct_[[ARR_CONSTRUCT]],
  // OFF-NODATAPAR-NEXT:    implicit=0, tool_info=(nil),
  // OFF-NODATAPAR-NEXT:    var_name=(null), bytes=20,
  // OFF-NODATAPAR-NEXT:    host_ptr=[[ARR1_HOST_PTR]],
  // OFF-NODATAPAR-NEXT:    device_ptr=[[ARR1_DEVICE_PTR]]
  // OFF-NODATAPAR-NEXT:  acc_api_info
  // OFF-NODATAPAR-NEXT:    device_api=0, valid_bytes=12,
  // OFF-NODATAPAR-NEXT:    device_type=acc_device_not_host
  // OFF-NODATAPAR-NEXT:acc_ev_free
  // OFF-NODATAPAR-NEXT:  acc_prof_info
  // OFF-NODATAPAR-NEXT:    event_type=9, valid_bytes=72, version=[[VERSION]],
  // OFF-NODATAPAR-NEXT:    device_type=acc_device_not_host, device_number=[[OFF_DEV]],
  // OFF-NODATAPAR-NEXT:    thread_id=[[THREAD_ID]], async=acc_async_sync, async_queue=[[ASYNC_QUEUE]],
  // OFF-NODATAPAR-NEXT:    src_file=[[SRC_FILE]], func_name=main,
  // OFF-NODATAPAR-NEXT:    line_no=[[ARR1_LINE_NO]], end_line_no=[[ARR1_END_LINE_NO]],
  // OFF-NODATAPAR-NEXT:    func_line_no=[[FUNC_LINE_NO]], func_end_line_no=[[FUNC_END_LINE_NO]]
  // OFF-NODATAPAR-NEXT:  acc_data_event_info
  // OFF-NODATAPAR-NEXT:    event_type=9, valid_bytes=56,
  // OFF-NODATAPAR-NEXT:    parent_construct=acc_construct_[[ARR_CONSTRUCT]],
  // OFF-NODATAPAR-NEXT:    implicit=0, tool_info=(nil),
  // OFF-NODATAPAR-NEXT:    var_name=(null), bytes=20,
  // OFF-NODATAPAR-NEXT:    host_ptr=[[ARR1_HOST_PTR]],
  // OFF-NODATAPAR-NEXT:    device_ptr=[[ARR1_DEVICE_PTR]]
  // OFF-NODATAPAR-NEXT:  acc_api_info
  // OFF-NODATAPAR-NEXT:    device_api=0, valid_bytes=12,
  // OFF-NODATAPAR-NEXT:    device_type=acc_device_not_host
  // OFF-NODATAPAR-NEXT:acc_ev_delete
  // OFF-NODATAPAR-NEXT:  acc_prof_info
  // OFF-NODATAPAR-NEXT:    event_type=7, valid_bytes=72, version=[[VERSION]],
  // OFF-NODATAPAR-NEXT:    device_type=acc_device_not_host, device_number=[[OFF_DEV]],
  // OFF-NODATAPAR-NEXT:    thread_id=[[THREAD_ID]], async=acc_async_sync, async_queue=[[ASYNC_QUEUE]],
  // OFF-NODATAPAR-NEXT:    src_file=[[SRC_FILE]], func_name=main,
  // OFF-NODATAPAR-NEXT:    line_no=[[ARR0_LINE_NO]], end_line_no=[[ARR0_END_LINE_NO]],
  // OFF-NODATAPAR-NEXT:    func_line_no=[[FUNC_LINE_NO]], func_end_line_no=[[FUNC_END_LINE_NO]]
  // OFF-NODATAPAR-NEXT:  acc_data_event_info
  // OFF-NODATAPAR-NEXT:    event_type=7, valid_bytes=56,
  // OFF-NODATAPAR-NEXT:    parent_construct=acc_construct_[[ARR_CONSTRUCT]],
  // OFF-NODATAPAR-NEXT:    implicit=0, tool_info=(nil),
  // OFF-NODATAPAR-NEXT:    var_name=(null), bytes=8,
  // OFF-NODATAPAR-NEXT:    host_ptr=[[ARR0_HOST_PTR]],
  // OFF-NODATAPAR-NEXT:    device_ptr=[[ARR0_DEVICE_PTR]]
  // OFF-NODATAPAR-NEXT:  acc_api_info
  // OFF-NODATAPAR-NEXT:    device_api=0, valid_bytes=12,
  // OFF-NODATAPAR-NEXT:    device_type=acc_device_not_host
  // OFF-NODATAPAR-NEXT:acc_ev_free
  // OFF-NODATAPAR-NEXT:  acc_prof_info
  // OFF-NODATAPAR-NEXT:    event_type=9, valid_bytes=72, version=[[VERSION]],
  // OFF-NODATAPAR-NEXT:    device_type=acc_device_not_host, device_number=[[OFF_DEV]],
  // OFF-NODATAPAR-NEXT:    thread_id=[[THREAD_ID]], async=acc_async_sync, async_queue=[[ASYNC_QUEUE]],
  // OFF-NODATAPAR-NEXT:    src_file=[[SRC_FILE]], func_name=main,
  // OFF-NODATAPAR-NEXT:    line_no=[[ARR0_LINE_NO]], end_line_no=[[ARR0_END_LINE_NO]],
  // OFF-NODATAPAR-NEXT:    func_line_no=[[FUNC_LINE_NO]], func_end_line_no=[[FUNC_END_LINE_NO]]
  // OFF-NODATAPAR-NEXT:  acc_data_event_info
  // OFF-NODATAPAR-NEXT:    event_type=9, valid_bytes=56,
  // OFF-NODATAPAR-NEXT:    parent_construct=acc_construct_[[ARR_CONSTRUCT]],
  // OFF-NODATAPAR-NEXT:    implicit=0, tool_info=(nil),
  // OFF-NODATAPAR-NEXT:    var_name=(null), bytes=8,
  // OFF-NODATAPAR-NEXT:    host_ptr=[[ARR0_HOST_PTR]],
  // OFF-NODATAPAR-NEXT:    device_ptr=[[ARR0_DEVICE_PTR]]
  // OFF-NODATAPAR-NEXT:  acc_api_info
  // OFF-NODATAPAR-NEXT:    device_api=0, valid_bytes=12,
  // OFF-NODATAPAR-NEXT:    device_type=acc_device_not_host
  // OFF-NODATAPAR-NEXT:acc_ev_exit_data_end
  // OFF-NODATAPAR-NEXT:  acc_prof_info
  // OFF-NODATAPAR-NEXT:    event_type=13, valid_bytes=72, version=[[VERSION]],
  // OFF-NODATAPAR-NEXT:    device_type=acc_device_not_host, device_number=[[OFF_DEV]],
  // OFF-NODATAPAR-NEXT:    thread_id=[[THREAD_ID]], async=acc_async_sync, async_queue=[[ASYNC_QUEUE]],
  // OFF-NODATAPAR-NEXT:    src_file=[[SRC_FILE]], func_name=main,
  // OFF-NODATAPAR-NEXT:    line_no=[[ARR0_LINE_NO]], end_line_no=[[ARR0_END_LINE_NO]],
  // OFF-NODATAPAR-NEXT:    func_line_no=[[FUNC_LINE_NO]], func_end_line_no=[[FUNC_END_LINE_NO]]
  // OFF-NODATAPAR-NEXT:  acc_other_event_info
  // OFF-NODATAPAR-NEXT:    event_type=13, valid_bytes=24,
  // OFF-NODATAPAR-NEXT:    parent_construct=acc_construct_[[ARR_CONSTRUCT]],
  // OFF-NODATAPAR-NEXT:    implicit=0, tool_info=(nil)
  // OFF-NODATAPAR-NEXT:  acc_api_info
  // OFF-NODATAPAR-NEXT:    device_api=0, valid_bytes=12,
  // OFF-NODATAPAR-NEXT:    device_type=acc_device_not_host

  // Exit data for arr1 if DATAPAR.
  //
  // OFF-DATAPAR-NEXT:acc_ev_exit_data_start
  // OFF-DATAPAR-NEXT:  acc_prof_info
  // OFF-DATAPAR-NEXT:    event_type=12, valid_bytes=72, version=[[VERSION]],
  // OFF-DATAPAR-NEXT:    device_type=acc_device_not_host, device_number=[[OFF_DEV]],
  // OFF-DATAPAR-NEXT:    thread_id=[[THREAD_ID]], async=acc_async_sync, async_queue=[[ASYNC_QUEUE]],
  // OFF-DATAPAR-NEXT:    src_file=[[SRC_FILE]], func_name=main,
  // OFF-DATAPAR-NEXT:    line_no=[[ARR1_LINE_NO]], end_line_no=[[ARR1_END_LINE_NO]],
  // OFF-DATAPAR-NEXT:    func_line_no=[[FUNC_LINE_NO]], func_end_line_no=[[FUNC_END_LINE_NO]]
  // OFF-DATAPAR-NEXT:  acc_other_event_info
  // OFF-DATAPAR-NEXT:    event_type=12, valid_bytes=24,
  // OFF-DATAPAR-NEXT:    parent_construct=acc_construct_[[ARR_CONSTRUCT]],
  // OFF-DATAPAR-NEXT:    implicit=0, tool_info=(nil)
  // OFF-DATAPAR-NEXT:  acc_api_info
  // OFF-DATAPAR-NEXT:    device_api=0, valid_bytes=12,
  // OFF-DATAPAR-NEXT:    device_type=acc_device_not_host
  // OFF-DATAPAR-NEXT:acc_ev_enqueue_download_start
  // OFF-DATAPAR-NEXT:  acc_prof_info
  // OFF-DATAPAR-NEXT:    event_type=22, valid_bytes=72, version=[[VERSION]],
  // OFF-DATAPAR-NEXT:    device_type=acc_device_not_host, device_number=[[OFF_DEV]],
  // OFF-DATAPAR-NEXT:    thread_id=[[THREAD_ID]], async=acc_async_sync, async_queue=[[ASYNC_QUEUE]],
  // OFF-DATAPAR-NEXT:    src_file=[[SRC_FILE]], func_name=main,
  // OFF-DATAPAR-NEXT:    line_no=[[ARR1_LINE_NO]], end_line_no=[[ARR1_END_LINE_NO]],
  // OFF-DATAPAR-NEXT:    func_line_no=[[FUNC_LINE_NO]], func_end_line_no=[[FUNC_END_LINE_NO]]
  // OFF-DATAPAR-NEXT:  acc_data_event_info
  // OFF-DATAPAR-NEXT:    event_type=22, valid_bytes=56,
  // OFF-DATAPAR-NEXT:    parent_construct=acc_construct_[[ARR_CONSTRUCT]],
  // OFF-DATAPAR-NEXT:    implicit=0, tool_info=(nil),
  // OFF-DATAPAR-NEXT:    var_name=(null), bytes=20,
  // OFF-DATAPAR-NEXT:    host_ptr=[[ARR1_HOST_PTR]],
  // OFF-DATAPAR-NEXT:    device_ptr=[[ARR1_DEVICE_PTR]]
  // OFF-DATAPAR-NEXT:  acc_api_info
  // OFF-DATAPAR-NEXT:    device_api=0, valid_bytes=12,
  // OFF-DATAPAR-NEXT:    device_type=acc_device_not_host
  // OFF-DATAPAR-NEXT:acc_ev_enqueue_download_end
  // OFF-DATAPAR-NEXT:  acc_prof_info
  // OFF-DATAPAR-NEXT:    event_type=23, valid_bytes=72, version=[[VERSION]],
  // OFF-DATAPAR-NEXT:    device_type=acc_device_not_host, device_number=[[OFF_DEV]],
  // OFF-DATAPAR-NEXT:    thread_id=[[THREAD_ID]], async=acc_async_sync, async_queue=[[ASYNC_QUEUE]],
  // OFF-DATAPAR-NEXT:    src_file=[[SRC_FILE]], func_name=main,
  // OFF-DATAPAR-NEXT:    line_no=[[ARR1_LINE_NO]], end_line_no=[[ARR1_END_LINE_NO]],
  // OFF-DATAPAR-NEXT:    func_line_no=[[FUNC_LINE_NO]], func_end_line_no=[[FUNC_END_LINE_NO]]
  // OFF-DATAPAR-NEXT:  acc_data_event_info
  // OFF-DATAPAR-NEXT:    event_type=23, valid_bytes=56,
  // OFF-DATAPAR-NEXT:    parent_construct=acc_construct_[[ARR_CONSTRUCT]],
  // OFF-DATAPAR-NEXT:    implicit=0, tool_info=(nil),
  // OFF-DATAPAR-NEXT:    var_name=(null), bytes=20,
  // OFF-DATAPAR-NEXT:    host_ptr=[[ARR1_HOST_PTR]],
  // OFF-DATAPAR-NEXT:    device_ptr=[[ARR1_DEVICE_PTR]]
  // OFF-DATAPAR-NEXT:  acc_api_info
  // OFF-DATAPAR-NEXT:    device_api=0, valid_bytes=12,
  // OFF-DATAPAR-NEXT:    device_type=acc_device_not_host
  // OFF-DATAPAR-NEXT:acc_ev_delete
  // OFF-DATAPAR-NEXT:  acc_prof_info
  // OFF-DATAPAR-NEXT:    event_type=7, valid_bytes=72, version=[[VERSION]],
  // OFF-DATAPAR-NEXT:    device_type=acc_device_not_host, device_number=[[OFF_DEV]],
  // OFF-DATAPAR-NEXT:    thread_id=[[THREAD_ID]], async=acc_async_sync, async_queue=[[ASYNC_QUEUE]],
  // OFF-DATAPAR-NEXT:    src_file=[[SRC_FILE]], func_name=main,
  // OFF-DATAPAR-NEXT:    line_no=[[ARR1_LINE_NO]], end_line_no=[[ARR1_END_LINE_NO]],
  // OFF-DATAPAR-NEXT:    func_line_no=[[FUNC_LINE_NO]], func_end_line_no=[[FUNC_END_LINE_NO]]
  // OFF-DATAPAR-NEXT:  acc_data_event_info
  // OFF-DATAPAR-NEXT:    event_type=7, valid_bytes=56,
  // OFF-DATAPAR-NEXT:    parent_construct=acc_construct_[[ARR_CONSTRUCT]],
  // OFF-DATAPAR-NEXT:    implicit=0, tool_info=(nil),
  // OFF-DATAPAR-NEXT:    var_name=(null), bytes=20,
  // OFF-DATAPAR-NEXT:    host_ptr=[[ARR1_HOST_PTR]],
  // OFF-DATAPAR-NEXT:    device_ptr=[[ARR1_DEVICE_PTR]]
  // OFF-DATAPAR-NEXT:  acc_api_info
  // OFF-DATAPAR-NEXT:    device_api=0, valid_bytes=12,
  // OFF-DATAPAR-NEXT:    device_type=acc_device_not_host
  // OFF-DATAPAR-NEXT:acc_ev_free
  // OFF-DATAPAR-NEXT:  acc_prof_info
  // OFF-DATAPAR-NEXT:    event_type=9, valid_bytes=72, version=[[VERSION]],
  // OFF-DATAPAR-NEXT:    device_type=acc_device_not_host, device_number=[[OFF_DEV]],
  // OFF-DATAPAR-NEXT:    thread_id=[[THREAD_ID]], async=acc_async_sync, async_queue=[[ASYNC_QUEUE]],
  // OFF-DATAPAR-NEXT:    src_file=[[SRC_FILE]], func_name=main,
  // OFF-DATAPAR-NEXT:    line_no=[[ARR1_LINE_NO]], end_line_no=[[ARR1_END_LINE_NO]],
  // OFF-DATAPAR-NEXT:    func_line_no=[[FUNC_LINE_NO]], func_end_line_no=[[FUNC_END_LINE_NO]]
  // OFF-DATAPAR-NEXT:  acc_data_event_info
  // OFF-DATAPAR-NEXT:    event_type=9, valid_bytes=56,
  // OFF-DATAPAR-NEXT:    parent_construct=acc_construct_[[ARR_CONSTRUCT]],
  // OFF-DATAPAR-NEXT:    implicit=0, tool_info=(nil),
  // OFF-DATAPAR-NEXT:    var_name=(null), bytes=20,
  // OFF-DATAPAR-NEXT:    host_ptr=[[ARR1_HOST_PTR]],
  // OFF-DATAPAR-NEXT:    device_ptr=[[ARR1_DEVICE_PTR]]
  // OFF-DATAPAR-NEXT:  acc_api_info
  // OFF-DATAPAR-NEXT:    device_api=0, valid_bytes=12,
  // OFF-DATAPAR-NEXT:    device_type=acc_device_not_host
  // OFF-DATAPAR-NEXT:acc_ev_exit_data_end
  // OFF-DATAPAR-NEXT:  acc_prof_info
  // OFF-DATAPAR-NEXT:    event_type=13, valid_bytes=72, version=[[VERSION]],
  // OFF-DATAPAR-NEXT:    device_type=acc_device_not_host, device_number=[[OFF_DEV]],
  // OFF-DATAPAR-NEXT:    thread_id=[[THREAD_ID]], async=acc_async_sync, async_queue=[[ASYNC_QUEUE]],
  // OFF-DATAPAR-NEXT:    src_file=[[SRC_FILE]], func_name=main,
  // OFF-DATAPAR-NEXT:    line_no=[[ARR1_LINE_NO]], end_line_no=[[ARR1_END_LINE_NO]],
  // OFF-DATAPAR-NEXT:    func_line_no=[[FUNC_LINE_NO]], func_end_line_no=[[FUNC_END_LINE_NO]]
  // OFF-DATAPAR-NEXT:  acc_other_event_info
  // OFF-DATAPAR-NEXT:    event_type=13, valid_bytes=24,
  // OFF-DATAPAR-NEXT:    parent_construct=acc_construct_data,
  // OFF-DATAPAR-NEXT:    implicit=0, tool_info=(nil)
  // OFF-DATAPAR-NEXT:  acc_api_info
  // OFF-DATAPAR-NEXT:    device_api=0, valid_bytes=12,
  // OFF-DATAPAR-NEXT:    device_type=acc_device_not_host

  // Exit data for arr0 if DATAPAR.
  //
  // OFF-DATAPAR-NEXT:acc_ev_exit_data_start
  // OFF-DATAPAR-NEXT:  acc_prof_info
  // OFF-DATAPAR-NEXT:    event_type=12, valid_bytes=72, version=[[VERSION]],
  // OFF-DATAPAR-NEXT:    device_type=acc_device_not_host, device_number=[[OFF_DEV]],
  // OFF-DATAPAR-NEXT:    thread_id=[[THREAD_ID]], async=acc_async_sync, async_queue=[[ASYNC_QUEUE]],
  // OFF-DATAPAR-NEXT:    src_file=[[SRC_FILE]], func_name=main,
  // OFF-DATAPAR-NEXT:    line_no=[[ARR0_LINE_NO]], end_line_no=[[ARR0_END_LINE_NO]],
  // OFF-DATAPAR-NEXT:    func_line_no=[[FUNC_LINE_NO]], func_end_line_no=[[FUNC_END_LINE_NO]]
  // OFF-DATAPAR-NEXT:  acc_other_event_info
  // OFF-DATAPAR-NEXT:    event_type=12, valid_bytes=24,
  // OFF-DATAPAR-NEXT:    parent_construct=acc_construct_data,
  // OFF-DATAPAR-NEXT:    implicit=0, tool_info=(nil)
  // OFF-DATAPAR-NEXT:  acc_api_info
  // OFF-DATAPAR-NEXT:    device_api=0, valid_bytes=12,
  // OFF-DATAPAR-NEXT:    device_type=acc_device_not_host
  // OFF-DATAPAR-NEXT:acc_ev_enqueue_download_start
  // OFF-DATAPAR-NEXT:  acc_prof_info
  // OFF-DATAPAR-NEXT:    event_type=22, valid_bytes=72, version=[[VERSION]],
  // OFF-DATAPAR-NEXT:    device_type=acc_device_not_host, device_number=[[OFF_DEV]],
  // OFF-DATAPAR-NEXT:    thread_id=[[THREAD_ID]], async=acc_async_sync, async_queue=[[ASYNC_QUEUE]],
  // OFF-DATAPAR-NEXT:    src_file=[[SRC_FILE]], func_name=main,
  // OFF-DATAPAR-NEXT:    line_no=[[ARR0_LINE_NO]], end_line_no=[[ARR0_END_LINE_NO]],
  // OFF-DATAPAR-NEXT:    func_line_no=[[FUNC_LINE_NO]], func_end_line_no=[[FUNC_END_LINE_NO]]
  // OFF-DATAPAR-NEXT:  acc_data_event_info
  // OFF-DATAPAR-NEXT:    event_type=22, valid_bytes=56,
  // OFF-DATAPAR-NEXT:    parent_construct=acc_construct_[[ARR_CONSTRUCT]],
  // OFF-DATAPAR-NEXT:    implicit=0, tool_info=(nil),
  // OFF-DATAPAR-NEXT:    var_name=(null), bytes=8,
  // OFF-DATAPAR-NEXT:    host_ptr=[[ARR0_HOST_PTR]],
  // OFF-DATAPAR-NEXT:    device_ptr=[[ARR0_DEVICE_PTR]]
  // OFF-DATAPAR-NEXT:  acc_api_info
  // OFF-DATAPAR-NEXT:    device_api=0, valid_bytes=12,
  // OFF-DATAPAR-NEXT:    device_type=acc_device_not_host
  // OFF-DATAPAR-NEXT:acc_ev_enqueue_download_end
  // OFF-DATAPAR-NEXT:  acc_prof_info
  // OFF-DATAPAR-NEXT:    event_type=23, valid_bytes=72, version=[[VERSION]],
  // OFF-DATAPAR-NEXT:    device_type=acc_device_not_host, device_number=[[OFF_DEV]],
  // OFF-DATAPAR-NEXT:    thread_id=[[THREAD_ID]], async=acc_async_sync, async_queue=[[ASYNC_QUEUE]],
  // OFF-DATAPAR-NEXT:    src_file=[[SRC_FILE]], func_name=main,
  // OFF-DATAPAR-NEXT:    line_no=[[ARR0_LINE_NO]], end_line_no=[[ARR0_END_LINE_NO]],
  // OFF-DATAPAR-NEXT:    func_line_no=[[FUNC_LINE_NO]], func_end_line_no=[[FUNC_END_LINE_NO]]
  // OFF-DATAPAR-NEXT:  acc_data_event_info
  // OFF-DATAPAR-NEXT:    event_type=23, valid_bytes=56,
  // OFF-DATAPAR-NEXT:    parent_construct=acc_construct_[[ARR_CONSTRUCT]],
  // OFF-DATAPAR-NEXT:    implicit=0, tool_info=(nil),
  // OFF-DATAPAR-NEXT:    var_name=(null), bytes=8,
  // OFF-DATAPAR-NEXT:    host_ptr=[[ARR0_HOST_PTR]],
  // OFF-DATAPAR-NEXT:    device_ptr=[[ARR0_DEVICE_PTR]]
  // OFF-DATAPAR-NEXT:  acc_api_info
  // OFF-DATAPAR-NEXT:    device_api=0, valid_bytes=12,
  // OFF-DATAPAR-NEXT:    device_type=acc_device_not_host
  // OFF-DATAPAR-NEXT:acc_ev_delete
  // OFF-DATAPAR-NEXT:  acc_prof_info
  // OFF-DATAPAR-NEXT:    event_type=7, valid_bytes=72, version=[[VERSION]],
  // OFF-DATAPAR-NEXT:    device_type=acc_device_not_host, device_number=[[OFF_DEV]],
  // OFF-DATAPAR-NEXT:    thread_id=[[THREAD_ID]], async=acc_async_sync, async_queue=[[ASYNC_QUEUE]],
  // OFF-DATAPAR-NEXT:    src_file=[[SRC_FILE]], func_name=main,
  // OFF-DATAPAR-NEXT:    line_no=[[ARR0_LINE_NO]], end_line_no=[[ARR0_END_LINE_NO]],
  // OFF-DATAPAR-NEXT:    func_line_no=[[FUNC_LINE_NO]], func_end_line_no=[[FUNC_END_LINE_NO]]
  // OFF-DATAPAR-NEXT:  acc_data_event_info
  // OFF-DATAPAR-NEXT:    event_type=7, valid_bytes=56,
  // OFF-DATAPAR-NEXT:    parent_construct=acc_construct_[[ARR_CONSTRUCT]],
  // OFF-DATAPAR-NEXT:    implicit=0, tool_info=(nil),
  // OFF-DATAPAR-NEXT:    var_name=(null), bytes=8,
  // OFF-DATAPAR-NEXT:    host_ptr=[[ARR0_HOST_PTR]],
  // OFF-DATAPAR-NEXT:    device_ptr=[[ARR0_DEVICE_PTR]]
  // OFF-DATAPAR-NEXT:  acc_api_info
  // OFF-DATAPAR-NEXT:    device_api=0, valid_bytes=12,
  // OFF-DATAPAR-NEXT:    device_type=acc_device_not_host
  // OFF-DATAPAR-NEXT:acc_ev_free
  // OFF-DATAPAR-NEXT:  acc_prof_info
  // OFF-DATAPAR-NEXT:    event_type=9, valid_bytes=72, version=[[VERSION]],
  // OFF-DATAPAR-NEXT:    device_type=acc_device_not_host, device_number=[[OFF_DEV]],
  // OFF-DATAPAR-NEXT:    thread_id=[[THREAD_ID]], async=acc_async_sync, async_queue=[[ASYNC_QUEUE]],
  // OFF-DATAPAR-NEXT:    src_file=[[SRC_FILE]], func_name=main,
  // OFF-DATAPAR-NEXT:    line_no=[[ARR0_LINE_NO]], end_line_no=[[ARR0_END_LINE_NO]],
  // OFF-DATAPAR-NEXT:    func_line_no=[[FUNC_LINE_NO]], func_end_line_no=[[FUNC_END_LINE_NO]]
  // OFF-DATAPAR-NEXT:  acc_data_event_info
  // OFF-DATAPAR-NEXT:    event_type=9, valid_bytes=56,
  // OFF-DATAPAR-NEXT:    parent_construct=acc_construct_[[ARR_CONSTRUCT]],
  // OFF-DATAPAR-NEXT:    implicit=0, tool_info=(nil),
  // OFF-DATAPAR-NEXT:    var_name=(null), bytes=8,
  // OFF-DATAPAR-NEXT:    host_ptr=[[ARR0_HOST_PTR]],
  // OFF-DATAPAR-NEXT:    device_ptr=[[ARR0_DEVICE_PTR]]
  // OFF-DATAPAR-NEXT:  acc_api_info
  // OFF-DATAPAR-NEXT:    device_api=0, valid_bytes=12,
  // OFF-DATAPAR-NEXT:    device_type=acc_device_not_host
  // OFF-DATAPAR-NEXT:acc_ev_exit_data_end
  // OFF-DATAPAR-NEXT:  acc_prof_info
  // OFF-DATAPAR-NEXT:    event_type=13, valid_bytes=72, version=[[VERSION]],
  // OFF-DATAPAR-NEXT:    device_type=acc_device_not_host, device_number=[[OFF_DEV]],
  // OFF-DATAPAR-NEXT:    thread_id=[[THREAD_ID]], async=acc_async_sync, async_queue=[[ASYNC_QUEUE]],
  // OFF-DATAPAR-NEXT:    src_file=[[SRC_FILE]], func_name=main,
  // OFF-DATAPAR-NEXT:    line_no=[[ARR0_LINE_NO]], end_line_no=[[ARR0_END_LINE_NO]],
  // OFF-DATAPAR-NEXT:    func_line_no=[[FUNC_LINE_NO]], func_end_line_no=[[FUNC_END_LINE_NO]]
  // OFF-DATAPAR-NEXT:  acc_other_event_info
  // OFF-DATAPAR-NEXT:    event_type=13, valid_bytes=24,
  // OFF-DATAPAR-NEXT:    parent_construct=acc_construct_[[ARR_CONSTRUCT]],
  // OFF-DATAPAR-NEXT:    implicit=0, tool_info=(nil)
  // OFF-DATAPAR-NEXT:  acc_api_info
  // OFF-DATAPAR-NEXT:    device_api=0, valid_bytes=12,
  // OFF-DATAPAR-NEXT:    device_type=acc_device_not_host

  // Parallel construct end if -DDIR=DIR_PAR.
  //
  // CHECK-PAR-NEXT:acc_ev_compute_construct_end
  // CHECK-PAR-NEXT:  acc_prof_info
  // CHECK-PAR-NEXT:    event_type=17, valid_bytes=72, version=[[VERSION]],
  //  HOST-PAR-NEXT:    device_type=acc_device_host, device_number=[[HOST_DEV]],
  //   OFF-PAR-NEXT:    device_type=acc_device_not_host, device_number=[[OFF_DEV]],
  // CHECK-PAR-NEXT:    thread_id=[[THREAD_ID]], async=acc_async_sync, async_queue=[[ASYNC_QUEUE]],
  // CHECK-PAR-NEXT:    src_file=[[SRC_FILE]], func_name=main,
  // CHECK-PAR-NEXT:    line_no=[[KERN_LINE_NO]], end_line_no=[[KERN_END_LINE_NO]],
  // CHECK-PAR-NEXT:    func_line_no=[[FUNC_LINE_NO]], func_end_line_no=[[FUNC_END_LINE_NO]]
  // CHECK-PAR-NEXT:  acc_other_event_info
  // CHECK-PAR-NEXT:    event_type=17, valid_bytes=24,
  // CHECK-PAR-NEXT:    parent_construct=acc_construct_parallel,
  // CHECK-PAR-NEXT:    implicit=0, tool_info=(nil)
  // CHECK-PAR-NEXT:  acc_api_info
  // CHECK-PAR-NEXT:    device_api=0, valid_bytes=12,
  //  HOST-PAR-NEXT:    device_type=acc_device_host
  //   OFF-PAR-NEXT:    device_type=acc_device_not_host

  // CHECK-NEXT:after kernel
  printf("after kernel\n");

  return 0;
#line 130000
}

// Device shutdown.
//
// OFF-NEXT:acc_ev_device_shutdown_start
// OFF-NEXT:  acc_prof_info
// OFF-NEXT:    event_type=3, valid_bytes=72, version=[[VERSION]],
// OFF-NEXT:    device_type=acc_device_not_host, device_number=[[OFF_DEV]],
// OFF-NEXT:    thread_id=[[THREAD_ID]], async=acc_async_sync, async_queue=[[ASYNC_QUEUE]],
// OFF-NEXT:    src_file=(null), func_name=(null),
// OFF-NEXT:    line_no=0, end_line_no=0,
// OFF-NEXT:    func_line_no=0, func_end_line_no=0
// OFF-NEXT:  acc_other_event_info
// OFF-NEXT:    event_type=3, valid_bytes=24,
// OFF-NEXT:    parent_construct=acc_construct_runtime_api,
// OFF-NEXT:    implicit=1, tool_info=(nil)
// OFF-NEXT:  acc_api_info
// OFF-NEXT:    device_api=0, valid_bytes=12,
// OFF-NEXT:    device_type=acc_device_not_host
// OFF-NEXT:acc_ev_device_shutdown_end
// OFF-NEXT:  acc_prof_info
// OFF-NEXT:    event_type=4, valid_bytes=72, version=[[VERSION]],
// OFF-NEXT:    device_type=acc_device_not_host, device_number=[[OFF_DEV]],
// OFF-NEXT:    thread_id=[[THREAD_ID]], async=acc_async_sync, async_queue=[[ASYNC_QUEUE]],
// OFF-NEXT:    src_file=(null), func_name=(null),
// OFF-NEXT:    line_no=0, end_line_no=0,
// OFF-NEXT:    func_line_no=0, func_end_line_no=0
// OFF-NEXT:  acc_other_event_info
// OFF-NEXT:    event_type=4, valid_bytes=24,
// OFF-NEXT:    parent_construct=acc_construct_runtime_api,
// OFF-NEXT:    implicit=1, tool_info=(nil)
// OFF-NEXT:  acc_api_info
// OFF-NEXT:    device_api=0, valid_bytes=12,
// OFF-NEXT:    device_type=acc_device_not_host

// Runtime shutdown.
//
// HOST-BEFORE-ENV-HASPAR-NEXT:acc_ev_runtime_shutdown
// HOST-BEFORE-ENV-HASPAR-NEXT:  acc_prof_info
// HOST-BEFORE-ENV-HASPAR-NEXT:    event_type=5, valid_bytes=72, version=[[VERSION]],
// HOST-BEFORE-ENV-HASPAR-NEXT:    device_type=acc_device_host, device_number=[[HOST_DEV]],
// HOST-BEFORE-ENV-HASPAR-NEXT:    thread_id=[[THREAD_ID]], async=acc_async_sync, async_queue=[[ASYNC_QUEUE]],
// HOST-BEFORE-ENV-HASPAR-NEXT:    src_file=(null), func_name=(null),
// HOST-BEFORE-ENV-HASPAR-NEXT:    line_no=0, end_line_no=0,
// HOST-BEFORE-ENV-HASPAR-NEXT:    func_line_no=0, func_end_line_no=0
// HOST-BEFORE-ENV-HASPAR-NEXT:  acc_other_event_info
// HOST-BEFORE-ENV-HASPAR-NEXT:    event_type=5, valid_bytes=24,
// HOST-BEFORE-ENV-HASPAR-NEXT:    parent_construct=acc_construct_runtime_api,
// HOST-BEFORE-ENV-HASPAR-NEXT:    implicit=1, tool_info=(nil)
// HOST-BEFORE-ENV-HASPAR-NEXT:  acc_api_info
// HOST-BEFORE-ENV-HASPAR-NEXT:    device_api=0, valid_bytes=12,
// HOST-BEFORE-ENV-HASPAR-NEXT:    device_type=acc_device_host
//
// OFF-BEFORE-ENV-NEXT:acc_ev_runtime_shutdown
// OFF-BEFORE-ENV-NEXT:  acc_prof_info
// OFF-BEFORE-ENV-NEXT:    event_type=5, valid_bytes=72, version=[[VERSION]],
// OFF-BEFORE-ENV-NEXT:    device_type=acc_device_host, device_number=[[HOST_DEV]],
// OFF-BEFORE-ENV-NEXT:    thread_id=[[THREAD_ID]], async=acc_async_sync, async_queue=[[ASYNC_QUEUE]],
// OFF-BEFORE-ENV-NEXT:    src_file=(null), func_name=(null),
// OFF-BEFORE-ENV-NEXT:    line_no=0, end_line_no=0,
// OFF-BEFORE-ENV-NEXT:    func_line_no=0, func_end_line_no=0
// OFF-BEFORE-ENV-NEXT:  acc_other_event_info
// OFF-BEFORE-ENV-NEXT:    event_type=5, valid_bytes=24,
// OFF-BEFORE-ENV-NEXT:    parent_construct=acc_construct_runtime_api,
// OFF-BEFORE-ENV-NEXT:    implicit=1, tool_info=(nil)
// OFF-BEFORE-ENV-NEXT:  acc_api_info
// OFF-BEFORE-ENV-NEXT:    device_api=0, valid_bytes=12,
// OFF-BEFORE-ENV-NEXT:    device_type=acc_device_host

// CHECK-NOT: {{.}}
