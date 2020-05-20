// Check no_create clauses on different constructs and with different values of
// -fopenacc-no-create-omp.  Diagnostics about no-alloc in the translation are
// tested in warn-acc-omp-map-no-alloc.c.
//
// The various cases covered here should be kept consistent with present.c.
// For example, a subarray that extends a subarray already present is
// consistently considered not present, so the present clause produces a runtime
// error and the no_create clause doesn't allocate.

// Check bad -fopenacc-no-create-omp values.
//
// RUN: %data bad-vals {
// RUN:   (val=foo)
// RUN:   (val=   )
// RUN: }
// RUN: %data bad-vals-cmds {
// RUN:   (cmd='%clang -fopenacc'    )
// RUN:   (cmd='%clang_cc1 -fopenacc')
// RUN:   (cmd='%clang'              )
// RUN:   (cmd='%clang_cc1'          )
// RUN: }
// RUN: %for bad-vals {
// RUN:   %for bad-vals-cmds {
// RUN:     not %[cmd] -fopenacc-no-create-omp=%[val] %s 2>&1 \
// RUN:     | FileCheck -check-prefix=BAD-VAL -DVAL=%[val] %s
// RUN:   }
// RUN: }
//
// BAD-VAL: error: invalid value '[[VAL]]' in '-fopenacc-no-create-omp=[[VAL]]'

// Define some interrelated data we use several times below.
//
// RUN: %data no-create-opts {
// RUN:   (no-create-opt=-Wno-openacc-omp-map-no-alloc                                    no-create-mt=no_alloc,alloc noAlloc-unless-alloc=NO-ALLOC noAllocDisjoint-unless-allocDisjoint=NO-ALLOC-DISJOINT noAllocExtends-unless-allocExtends=NO-ALLOC-EXTENDS noAllocConcat2-unless-allocConcat2=NO-ALLOC-CONCAT2 not-if-alloc=   )
// RUN:   (no-create-opt='-fopenacc-no-create-omp=no_alloc -Wno-openacc-omp-map-no-alloc' no-create-mt=no_alloc,alloc noAlloc-unless-alloc=NO-ALLOC noAllocDisjoint-unless-allocDisjoint=NO-ALLOC-DISJOINT noAllocExtends-unless-allocExtends=NO-ALLOC-EXTENDS noAllocConcat2-unless-allocConcat2=NO-ALLOC-CONCAT2 not-if-alloc=   )
// RUN:   (no-create-opt=-fopenacc-no-create-omp=alloc                                    no-create-mt=alloc          noAlloc-unless-alloc=ALLOC    noAllocDisjoint-unless-allocDisjoint=ALLOC-DISJOINT    noAllocExtends-unless-allocExtends=ALLOC-EXTENDS    noAllocConcat2-unless-allocConcat2=ALLOC-CONCAT2    not-if-alloc=not)
// RUN: }
// RUN: %data tgts {
// RUN:   (run-if=                tgt-cflags=                                     present-unless-host=HOST    noAlloc-unless-hostOrAlloc=HOST                    noAllocDisjoint-unless-hostOrAllocDisjoint=HOST                                    noAllocExtends-unless-hostOrAllocExtends=HOST                                  noAllocConcat2-unless-hostOrAllocConcat2=HOST                                  not-if-off-and-alloc=               )
// RUN:   (run-if=%run-if-x86_64  tgt-cflags=-fopenmp-targets=%run-x86_64-triple  present-unless-host=PRESENT noAlloc-unless-hostOrAlloc=%[noAlloc-unless-alloc] noAllocDisjoint-unless-hostOrAllocDisjoint=%[noAllocDisjoint-unless-allocDisjoint] noAllocExtends-unless-hostOrAllocExtends=%[noAllocExtends-unless-allocExtends] noAllocConcat2-unless-hostOrAllocConcat2=%[noAllocConcat2-unless-allocConcat2] not-if-off-and-alloc=%[not-if-alloc])
// RUN:   (run-if=%run-if-ppc64le tgt-cflags=-fopenmp-targets=%run-ppc64le-triple present-unless-host=PRESENT noAlloc-unless-hostOrAlloc=%[noAlloc-unless-alloc] noAllocDisjoint-unless-hostOrAllocDisjoint=%[noAllocDisjoint-unless-allocDisjoint] noAllocExtends-unless-hostOrAllocExtends=%[noAllocExtends-unless-allocExtends] noAllocConcat2-unless-hostOrAllocConcat2=%[noAllocConcat2-unless-allocConcat2] not-if-off-and-alloc=%[not-if-alloc])
// RUN:   (run-if=%run-if-nvptx64 tgt-cflags=-fopenmp-targets=%run-nvptx64-triple present-unless-host=PRESENT noAlloc-unless-hostOrAlloc=%[noAlloc-unless-alloc] noAllocDisjoint-unless-hostOrAllocDisjoint=%[noAllocDisjoint-unless-allocDisjoint] noAllocExtends-unless-hostOrAllocExtends=%[noAllocExtends-unless-allocExtends] noAllocConcat2-unless-hostOrAllocConcat2=%[noAllocConcat2-unless-allocConcat2] not-if-off-and-alloc=%[not-if-alloc])
// RUN: }
//      # "acc parallel loop" should be about the same as "acc parallel", so a
//      # few cases are probably sufficient for it.
// RUN: %data cases {
// RUN:   (case=CASE_DATA_SCALAR_PRESENT             exe=%[present-unless-host]                        data-or-par=DATA  not-if-fail=                       )
// RUN:   (case=CASE_DATA_SCALAR_ABSENT              exe=%[noAlloc-unless-hostOrAlloc]                 data-or-par=DATA  not-if-fail=                       )
// RUN:   (case=CASE_DATA_ARRAY_PRESENT              exe=%[present-unless-host]                        data-or-par=DATA  not-if-fail=                       )
// RUN:   (case=CASE_DATA_ARRAY_ABSENT               exe=%[noAlloc-unless-hostOrAlloc]                 data-or-par=DATA  not-if-fail=                       )
// RUN:   (case=CASE_DATA_SUBARRAY_PRESENT           exe=%[present-unless-host]                        data-or-par=DATA  not-if-fail=                       )
// RUN:   (case=CASE_DATA_SUBARRAY_DISJOINT          exe=%[noAllocDisjoint-unless-hostOrAllocDisjoint] data-or-par=DATA  not-if-fail=                       )
// RUN:   (case=CASE_DATA_SUBARRAY_OVERLAP_START     exe=%[noAllocExtends-unless-hostOrAllocExtends]   data-or-par=DATA  not-if-fail=%[not-if-off-and-alloc])
// RUN:   (case=CASE_DATA_SUBARRAY_OVERLAP_END       exe=%[noAllocExtends-unless-hostOrAllocExtends]   data-or-par=DATA  not-if-fail=%[not-if-off-and-alloc])
// RUN:   (case=CASE_DATA_SUBARRAY_CONCAT2           exe=%[noAllocConcat2-unless-hostOrAllocConcat2]   data-or-par=DATA  not-if-fail=%[not-if-off-and-alloc])
// RUN:   (case=CASE_PARALLEL_SCALAR_PRESENT         exe=%[present-unless-host]                        data-or-par=PAR   not-if-fail=                       )
// RUN:   (case=CASE_PARALLEL_SCALAR_ABSENT          exe=%[noAlloc-unless-hostOrAlloc]                 data-or-par=PAR   not-if-fail=                       )
// RUN:   (case=CASE_PARALLEL_ARRAY_PRESENT          exe=%[present-unless-host]                        data-or-par=PAR   not-if-fail=                       )
// RUN:   (case=CASE_PARALLEL_ARRAY_ABSENT           exe=%[noAlloc-unless-hostOrAlloc]                 data-or-par=PAR   not-if-fail=                       )
// RUN:   (case=CASE_PARALLEL_SUBARRAY_PRESENT       exe=%[present-unless-host]                        data-or-par=PAR   not-if-fail=                       )
// RUN:   (case=CASE_PARALLEL_SUBARRAY_DISJOINT      exe=%[noAllocDisjoint-unless-hostOrAllocDisjoint] data-or-par=PAR   not-if-fail=                       )
// RUN:   (case=CASE_PARALLEL_SUBARRAY_OVERLAP_START exe=%[noAllocExtends-unless-hostOrAllocExtends]   data-or-par=PAR   not-if-fail=%[not-if-off-and-alloc])
// RUN:   (case=CASE_PARALLEL_SUBARRAY_OVERLAP_END   exe=%[noAllocExtends-unless-hostOrAllocExtends]   data-or-par=PAR   not-if-fail=%[not-if-off-and-alloc])
// RUN:   (case=CASE_PARALLEL_SUBARRAY_CONCAT2       exe=%[noAllocConcat2-unless-hostOrAllocConcat2]   data-or-par=PAR   not-if-fail=%[not-if-off-and-alloc])
// RUN:   (case=CASE_PARALLEL_LOOP_SCALAR_PRESENT    exe=%[present-unless-host]                        data-or-par=PAR   not-if-fail=                       )
// RUN:   (case=CASE_PARALLEL_LOOP_SCALAR_ABSENT     exe=%[noAlloc-unless-hostOrAlloc]                 data-or-par=PAR   not-if-fail=                       )
// RUN:   (case=CASE_CONST_PRESENT                   exe=%[present-unless-host]                        data-or-par=PAR   not-if-fail=                       )
// RUN:   (case=CASE_CONST_ABSENT                    exe=%[noAlloc-unless-hostOrAlloc]                 data-or-par=PAR   not-if-fail=                       )
// RUN: }

// Check -ast-dump before and after AST serialization.
//
// We include dump checking on only a few representative cases, which should be
// more than sufficient to show it's working for the no_create clause.
//
// RUN: %for no-create-opts {
// RUN:   %clang -Xclang -verify -Xclang -ast-dump -fsyntax-only -fopenacc %s \
// RUN:          %[no-create-opt] \
// RUN:   | FileCheck -check-prefixes=DMP %s
// RUN:   %clang -Xclang -verify -fopenacc -emit-ast -o %t.ast %s \
// RUN:          %[no-create-opt]
// RUN:   %clang_cc1 -ast-dump-all %t.ast \
// RUN:   | FileCheck -check-prefixes=DMP %s
// RUN: }

// Check -ast-print and -fopenacc[-ast]-print.
//
// We include print checking on only a few representative cases, which should be
// more than sufficient to show it's working for the no_create clause.
//
// RUN: %clang -Xclang -verify -Xclang -ast-print -fsyntax-only %s \
// RUN: | FileCheck -check-prefixes=PRT,PRT-NOACC %s
//
// TODO: If lit were to support %for inside a %data, we could iterate prt-opts
// within prt-args after the first prt-args iteration, significantly shortening
// the prt-args definition.
//
// Strip comments and blank lines so checking -fopenacc-print output is easier.
// RUN: echo "// expected""-no-diagnostics" > %t-acc.c
// RUN: grep -v '^ *\(//.*\)\?$' %s | sed 's,//.*,,' >> %t-acc.c
//
// RUN: %data prt-opts {
// RUN:   (prt-opt=-fopenacc-ast-print)
// RUN:   (prt-opt=-fopenacc-print    )
// RUN: }
// RUN: %data prt-args {
// RUN:   (prt='-Xclang -ast-print -fsyntax-only -fopenacc' prt-chk=PRT,PRT-A       )
// RUN:   (prt=-fopenacc-ast-print=acc                      prt-chk=PRT,PRT-A       )
// RUN:   (prt=-fopenacc-ast-print=omp                      prt-chk=PRT,PRT-O       )
// RUN:   (prt=-fopenacc-ast-print=acc-omp                  prt-chk=PRT,PRT-A,PRT-AO)
// RUN:   (prt=-fopenacc-ast-print=omp-acc                  prt-chk=PRT,PRT-O,PRT-OA)
// RUN:   (prt=-fopenacc-print=acc                          prt-chk=PRT,PRT-A       )
// RUN:   (prt=-fopenacc-print=omp                          prt-chk=PRT,PRT-O       )
// RUN:   (prt=-fopenacc-print=acc-omp                      prt-chk=PRT,PRT-A,PRT-AO)
// RUN:   (prt=-fopenacc-print=omp-acc                      prt-chk=PRT,PRT-O,PRT-OA)
// RUN: }
// RUN: %for no-create-opts {
// RUN:   %for prt-args {
// RUN:     %clang -Xclang -verify %[prt] %[no-create-opt] %t-acc.c \
// RUN:     | FileCheck -check-prefixes=%[prt-chk] \
// RUN:                 -DNO_CREATE_MT=%[no-create-mt] %s
// RUN:   }
// RUN: }

// Check -ast-print after AST serialization.
//
// Some data related to printing (where to print comments about discarded
// directives) is serialized and deserialized, so it's worthwhile to try all
// OpenACC printing modes.
//
// RUN: %for no-create-opts {
// RUN:   %clang -Xclang -verify -fopenacc %[no-create-opt] -emit-ast \
// RUN:          -o %t.ast %t-acc.c
// RUN:   %for prt-args {
// RUN:     %clang %[prt] %t.ast 2>&1 \
// RUN:     | FileCheck -check-prefixes=%[prt-chk] \
// RUN:                 -DNO_CREATE_MT=%[no-create-mt] %s
// RUN:   }
// RUN: }

// Can we print the OpenMP source code, compile, and run it successfully?
//
// We don't normally bother to check this for offloading, but the no_create
// clause has no effect when not offloading (that is, for shared memory), and
// one of the main issues with the no_create clause is the various ways it can
// be translated so it can be used in source-to-source when targeting other
// compilers.  That is, we want to be sure source-to-source mode produces
// working translations of the no_create clause in all cases.
//
// RUN: %for no-create-opts {
// RUN:   %for tgts {
// RUN:     %for prt-opts {
// RUN:       %[run-if] %clang -Xclang -verify %[prt-opt]=omp \
// RUN:                 %[no-create-opt] %s > %t-omp.c
// RUN:       %[run-if] echo "// expected""-no-diagnostics" >> %t-omp.c
// RUN:       %[run-if] %clang -Xclang -verify -fopenmp %fopenmp-version \
// RUN:                 %[tgt-cflags] -o %t.exe %t-omp.c
// RUN:       %for cases {
// RUN:         %[run-if] %[not-if-fail] env LIBOMPTARGET_DEBUG=1 %t.exe \
// RUN:                                  %[case] > %t.out 2>&1
// RUN:         %[run-if] FileCheck -input-file %t.out -allow-empty %s \
// RUN:           -check-prefixes=EXE-%[exe],EXE-%[exe]-%[data-or-par] \
// RUN:           -implicit-check-not=Entering
// RUN:       }
// RUN:     }
// RUN:   }
// RUN: }

// Check execution with normal compilation.
//
// RUN: %for no-create-opts {
// RUN:   %for tgts {
// RUN:     %[run-if] %clang -Xclang -verify -fopenacc %[no-create-opt] \
// RUN:               %[tgt-cflags] -o %t.exe %s
// RUN:     rm -f %t.actual-cases && touch %t.actual-cases
// RUN:     %for cases {
// RUN:       %[run-if] %[not-if-fail] env LIBOMPTARGET_DEBUG=1 %t.exe \
// RUN:                                %[case] > %t.out 2>&1
// RUN:       %[run-if] FileCheck -input-file %t.out -allow-empty %s \
// RUN:         -check-prefixes=EXE-%[exe],EXE-%[exe]-%[data-or-par] \
// RUN:         -implicit-check-not=Entering
// RUN:       echo '%[case]' >> %t.actual-cases
// RUN:     }
// RUN:   }
// RUN: }

// Make sure %data cases didn't omit any cases defined in the code.
//
// RUN: %t.exe -dump-cases > %t.expected-cases
// RUN: diff -u %t.expected-cases %t.actual-cases >&2

// END.

// expected-no-diagnostics

#include <stdio.h>
#include <string.h>

#define FOREACH_CASE(Macro)                   \
  Macro(CASE_DATA_SCALAR_PRESENT)             \
  Macro(CASE_DATA_SCALAR_ABSENT)              \
  Macro(CASE_DATA_ARRAY_PRESENT)              \
  Macro(CASE_DATA_ARRAY_ABSENT)               \
  Macro(CASE_DATA_SUBARRAY_PRESENT)           \
  Macro(CASE_DATA_SUBARRAY_DISJOINT)          \
  Macro(CASE_DATA_SUBARRAY_OVERLAP_START)     \
  Macro(CASE_DATA_SUBARRAY_OVERLAP_END)       \
  Macro(CASE_DATA_SUBARRAY_CONCAT2)           \
  Macro(CASE_PARALLEL_SCALAR_PRESENT)         \
  Macro(CASE_PARALLEL_SCALAR_ABSENT)          \
  Macro(CASE_PARALLEL_ARRAY_PRESENT)          \
  Macro(CASE_PARALLEL_ARRAY_ABSENT)           \
  Macro(CASE_PARALLEL_SUBARRAY_PRESENT)       \
  Macro(CASE_PARALLEL_SUBARRAY_DISJOINT)      \
  Macro(CASE_PARALLEL_SUBARRAY_OVERLAP_START) \
  Macro(CASE_PARALLEL_SUBARRAY_OVERLAP_END)   \
  Macro(CASE_PARALLEL_SUBARRAY_CONCAT2)       \
  Macro(CASE_PARALLEL_LOOP_SCALAR_PRESENT)    \
  Macro(CASE_PARALLEL_LOOP_SCALAR_ABSENT)     \
  Macro(CASE_CONST_PRESENT)                   \
  Macro(CASE_CONST_ABSENT)

enum Case {
#define AddCase(CaseName) \
  CaseName,
FOREACH_CASE(AddCase)
  CASE_END
#undef AddCase
};

const char *CaseNames[] = {
#define AddCase(CaseName) \
  #CaseName,
FOREACH_CASE(AddCase)
#undef AddCase
};

int main(int argc, char *argv[]) {
  if (argc != 2) {
    fprintf(stderr, "expected one argument\n");
    return 1;
  }
  if (!strcmp(argv[1], "-dump-cases")) {
    for (int i = 0; i < CASE_END; ++i)
      printf("%s\n", CaseNames[i]);
    return 0;
  }
  enum Case selectedCase;
  for (selectedCase = 0; selectedCase < CASE_END; ++selectedCase) {
    if (!strcmp(argv[1], CaseNames[selectedCase]))
      break;
  }
  if (selectedCase == CASE_END) {
    fprintf(stderr, "unexpected case: %s\n", argv[1]);
    return 1;
  }

  // DMP-LABEL: SwitchStmt
  // PRT-LABEL: switch (selectedCase)
  switch (selectedCase) {
  // DMP-LABEL: CASE_DATA_SCALAR_PRESENT
  //       DMP: ACCDataDirective
  //  DMP-NEXT:   ACCCopyClause
  //  DMP-NEXT:     DeclRefExpr {{.*}} 'x' 'int'
  //  DMP-NEXT:   impl: OMPTargetDataDirective
  //  DMP-NEXT:     OMPMapClause
  //  DMP-NEXT:       DeclRefExpr {{.*}} 'x' 'int'
  //       DMP:   ACCDataDirective
  //  DMP-NEXT:     ACCNoCreateClause
  //  DMP-NEXT:       DeclRefExpr {{.*}} 'x' 'int'
  //  DMP-NEXT:     impl: OMPTargetDataDirective
  //  DMP-NEXT:       OMPMapClause
  //  DMP-NEXT:         DeclRefExpr {{.*}} 'x' 'int'
  //
  //   PRT-LABEL: case CASE_DATA_SCALAR_PRESENT:
  //    PRT-NEXT: {
  //    PRT-NEXT:   int x;
  //
  //  PRT-A-NEXT:   #pragma acc data copy(x){{$}}
  // PRT-AO-NEXT:   // #pragma omp target data map(tofrom: x){{$}}
  //  PRT-A-NEXT:   #pragma acc data no_create(x){{$}}
  // PRT-AO-NEXT:   // #pragma omp target data map([[NO_CREATE_MT]]: x){{$}}
  //
  //  PRT-O-NEXT:   #pragma omp target data map(tofrom: x){{$}}
  // PRT-OA-NEXT:   // #pragma acc data copy(x){{$}}
  //  PRT-O-NEXT:   #pragma omp target data map([[NO_CREATE_MT]]: x){{$}}
  // PRT-OA-NEXT:   // #pragma acc data no_create(x){{$}}
  //
  //    PRT-NEXT:   ;
  //    PRT-NEXT:   break;
  //    PRT-NEXT: }
  case CASE_DATA_SCALAR_PRESENT:
  {
    int x;
    #pragma acc data copy(x)
    #pragma acc data no_create(x)
    ;
    break;
  }

  // DMP-LABEL: CASE_DATA_SCALAR_ABSENT
  //       DMP: ACCDataDirective
  //  DMP-NEXT:   ACCNoCreateClause
  //  DMP-NEXT:     DeclRefExpr {{.*}} 'x' 'int'
  //  DMP-NEXT:   impl: OMPTargetDataDirective
  //  DMP-NEXT:     OMPMapClause
  //  DMP-NEXT:       DeclRefExpr {{.*}} 'x' 'int'
  //
  //   PRT-LABEL: case CASE_DATA_SCALAR_ABSENT:
  //    PRT-NEXT: {
  //    PRT-NEXT:   int x;
  //  PRT-A-NEXT:   #pragma acc data no_create(x){{$}}
  // PRT-AO-NEXT:   // #pragma omp target data map([[NO_CREATE_MT]]: x){{$}}
  //  PRT-O-NEXT:   #pragma omp target data map([[NO_CREATE_MT]]: x){{$}}
  // PRT-OA-NEXT:   // #pragma acc data no_create(x){{$}}
  //    PRT-NEXT:   ;
  //    PRT-NEXT:   break;
  //    PRT-NEXT: }
  case CASE_DATA_SCALAR_ABSENT:
  {
    int x;
    #pragma acc data no_create(x)
    ;
    break;
  }

  //   PRT-LABEL: case CASE_DATA_ARRAY_PRESENT:
  //    PRT-NEXT: {
  //    PRT-NEXT:   int arr[3];
  //
  //  PRT-A-NEXT:   #pragma acc data copy(arr){{$}}
  // PRT-AO-NEXT:   // #pragma omp target data map(tofrom: arr){{$}}
  //  PRT-A-NEXT:   #pragma acc data no_create(arr){{$}}
  // PRT-AO-NEXT:   // #pragma omp target data map([[NO_CREATE_MT]]: arr){{$}}
  //
  //  PRT-O-NEXT:   #pragma omp target data map(tofrom: arr){{$}}
  // PRT-OA-NEXT:   // #pragma acc data copy(arr){{$}}
  //  PRT-O-NEXT:   #pragma omp target data map([[NO_CREATE_MT]]: arr){{$}}
  // PRT-OA-NEXT:   // #pragma acc data no_create(arr){{$}}
  //
  //    PRT-NEXT:   ;
  //    PRT-NEXT:   break;
  //    PRT-NEXT: }
  case CASE_DATA_ARRAY_PRESENT:
  {
    int arr[3];
    #pragma acc data copy(arr)
    #pragma acc data no_create(arr)
    ;
    break;
  }

  //   PRT-LABEL: case CASE_DATA_ARRAY_ABSENT:
  //    PRT-NEXT: {
  //    PRT-NEXT:   int arr[3];
  //  PRT-A-NEXT:   #pragma acc data no_create(arr){{$}}
  // PRT-AO-NEXT:   // #pragma omp target data map([[NO_CREATE_MT]]: arr){{$}}
  //  PRT-O-NEXT:   #pragma omp target data map([[NO_CREATE_MT]]: arr){{$}}
  // PRT-OA-NEXT:   // #pragma acc data no_create(arr){{$}}
  //    PRT-NEXT:   ;
  //    PRT-NEXT:   break;
  //    PRT-NEXT: }
  case CASE_DATA_ARRAY_ABSENT:
  {
    int arr[3];
    #pragma acc data no_create(arr)
    ;
    break;
  }

  //   PRT-LABEL: case CASE_DATA_SUBARRAY_PRESENT:
  //    PRT-NEXT: {
  //    PRT-NEXT:   int all[10], same[10], beg[10], mid[10], end[10];
  //
  //  PRT-A-NEXT:   #pragma acc data copy(all,same[3:6],beg[2:5],mid[1:8],end[0:5])
  // PRT-AO-NEXT:   // #pragma omp target data map(tofrom: all,same[3:6],beg[2:5],mid[1:8],end[0:5])
  //  PRT-A-NEXT:   #pragma acc data no_create(all[0:10],same[3:6],beg[2:2],mid[3:3],end[4:1])
  // PRT-AO-NEXT:   // #pragma omp target data map([[NO_CREATE_MT]]: all[0:10],same[3:6],beg[2:2],mid[3:3],end[4:1])
  //
  //  PRT-O-NEXT:   #pragma omp target data map(tofrom: all,same[3:6],beg[2:5],mid[1:8],end[0:5])
  // PRT-OA-NEXT:   // #pragma acc data copy(all,same[3:6],beg[2:5],mid[1:8],end[0:5])
  //  PRT-O-NEXT:   #pragma omp target data map([[NO_CREATE_MT]]: all[0:10],same[3:6],beg[2:2],mid[3:3],end[4:1])
  // PRT-OA-NEXT:   // #pragma acc data no_create(all[0:10],same[3:6],beg[2:2],mid[3:3],end[4:1])
  //
  //    PRT-NEXT:   ;
  //    PRT-NEXT:   break;
  //    PRT-NEXT: }
  case CASE_DATA_SUBARRAY_PRESENT:
  {
    int all[10], same[10], beg[10], mid[10], end[10];
    #pragma acc data copy(all,same[3:6],beg[2:5],mid[1:8],end[0:5])
    #pragma acc data no_create(all[0:10],same[3:6],beg[2:2],mid[3:3],end[4:1])
    ;
    break;
  }

  //   PRT-LABEL: case CASE_DATA_SUBARRAY_DISJOINT:
  //    PRT-NEXT: {
  //    PRT-NEXT:   int arr[4];
  //
  //  PRT-A-NEXT:   #pragma acc data copy(arr[0:2]){{$}}
  // PRT-AO-NEXT:   // #pragma omp target data map(tofrom: arr[0:2]){{$}}
  //  PRT-A-NEXT:   #pragma acc data no_create(arr[2:2]){{$}}
  // PRT-AO-NEXT:   // #pragma omp target data map([[NO_CREATE_MT]]: arr[2:2]){{$}}
  //
  //  PRT-O-NEXT:   #pragma omp target data map(tofrom: arr[0:2]){{$}}
  // PRT-OA-NEXT:   // #pragma acc data copy(arr[0:2]){{$}}
  //  PRT-O-NEXT:   #pragma omp target data map([[NO_CREATE_MT]]: arr[2:2]){{$}}
  // PRT-OA-NEXT:   // #pragma acc data no_create(arr[2:2]){{$}}
  //
  //    PRT-NEXT:   ;
  //    PRT-NEXT:   break;
  //    PRT-NEXT: }
  case CASE_DATA_SUBARRAY_DISJOINT:
  {
    int arr[4];
    #pragma acc data copy(arr[0:2])
    #pragma acc data no_create(arr[2:2])
    ;
    break;
  }

  //   PRT-LABEL: case CASE_DATA_SUBARRAY_OVERLAP_START:
  //    PRT-NEXT: {
  //    PRT-NEXT:   int arr[5];
  //
  //  PRT-A-NEXT:   #pragma acc data copyin(arr[1:2]){{$}}
  // PRT-AO-NEXT:   // #pragma omp target data map(to: arr[1:2]){{$}}
  //  PRT-A-NEXT:   #pragma acc data no_create(arr[0:2]){{$}}
  // PRT-AO-NEXT:   // #pragma omp target data map([[NO_CREATE_MT]]: arr[0:2]){{$}}
  //
  //  PRT-O-NEXT:   #pragma omp target data map(to: arr[1:2]){{$}}
  // PRT-OA-NEXT:   // #pragma acc data copyin(arr[1:2]){{$}}
  //  PRT-O-NEXT:   #pragma omp target data map([[NO_CREATE_MT]]: arr[0:2]){{$}}
  // PRT-OA-NEXT:   // #pragma acc data no_create(arr[0:2]){{$}}
  //
  //    PRT-NEXT:   ;
  //    PRT-NEXT:   break;
  //    PRT-NEXT: }
  case CASE_DATA_SUBARRAY_OVERLAP_START:
  {
    int arr[5];
    #pragma acc data copyin(arr[1:2])
    #pragma acc data no_create(arr[0:2])
    ;
    break;
  }

  //   PRT-LABEL: case CASE_DATA_SUBARRAY_OVERLAP_END:
  //    PRT-NEXT: {
  //    PRT-NEXT:   int arr[5];
  //
  //  PRT-A-NEXT:   #pragma acc data copyin(arr[1:2]){{$}}
  // PRT-AO-NEXT:   // #pragma omp target data map(to: arr[1:2]){{$}}
  //  PRT-A-NEXT:   #pragma acc data no_create(arr[2:2]){{$}}
  // PRT-AO-NEXT:   // #pragma omp target data map([[NO_CREATE_MT]]: arr[2:2]){{$}}
  //
  //  PRT-O-NEXT:   #pragma omp target data map(to: arr[1:2]){{$}}
  // PRT-OA-NEXT:   // #pragma acc data copyin(arr[1:2]){{$}}
  //  PRT-O-NEXT:   #pragma omp target data map([[NO_CREATE_MT]]: arr[2:2]){{$}}
  // PRT-OA-NEXT:   // #pragma acc data no_create(arr[2:2]){{$}}
  //
  //    PRT-NEXT:   ;
  //    PRT-NEXT:   break;
  //    PRT-NEXT: }
  case CASE_DATA_SUBARRAY_OVERLAP_END:
  {
    int arr[5];
    #pragma acc data copyin(arr[1:2])
    #pragma acc data no_create(arr[2:2])
    ;
    break;
  }

  //   PRT-LABEL: case CASE_DATA_SUBARRAY_CONCAT2:
  //    PRT-NEXT: {
  //    PRT-NEXT:   int arr[4];
  //
  //  PRT-A-NEXT:   #pragma acc data copyout(arr[0:2]){{$}}
  // PRT-AO-NEXT:   // #pragma omp target data map(from: arr[0:2]){{$}}
  //  PRT-A-NEXT:   #pragma acc data copy(arr[2:2]){{$}}
  // PRT-AO-NEXT:   // #pragma omp target data map(tofrom: arr[2:2]){{$}}
  //  PRT-A-NEXT:   #pragma acc data no_create(arr[0:4]){{$}}
  // PRT-AO-NEXT:   // #pragma omp target data map([[NO_CREATE_MT]]: arr[0:4]){{$}}
  //
  //  PRT-O-NEXT:   #pragma omp target data map(from: arr[0:2]){{$}}
  // PRT-OA-NEXT:   // #pragma acc data copyout(arr[0:2]){{$}}
  //  PRT-O-NEXT:   #pragma omp target data map(tofrom: arr[2:2]){{$}}
  // PRT-OA-NEXT:   // #pragma acc data copy(arr[2:2]){{$}}
  //  PRT-O-NEXT:   #pragma omp target data map([[NO_CREATE_MT]]: arr[0:4]){{$}}
  // PRT-OA-NEXT:   // #pragma acc data no_create(arr[0:4]){{$}}
  //
  //    PRT-NEXT:   ;
  //    PRT-NEXT:   break;
  //    PRT-NEXT: }
  case CASE_DATA_SUBARRAY_CONCAT2:
  {
    int arr[4];
    #pragma acc data copyout(arr[0:2])
    #pragma acc data copy(arr[2:2])
    #pragma acc data no_create(arr[0:4])
    ;
    break;
  }

  // DMP-LABEL: CASE_PARALLEL_SCALAR_PRESENT
  //       DMP: ACCDataDirective
  //  DMP-NEXT:   ACCCopyClause
  //  DMP-NEXT:     DeclRefExpr {{.*}} 'x' 'int'
  //  DMP-NEXT:   impl: OMPTargetDataDirective
  //  DMP-NEXT:     OMPMapClause
  //  DMP-NEXT:       DeclRefExpr {{.*}} 'x' 'int'
  //       DMP: ACCParallelDirective
  //  DMP-NEXT:   ACCNoCreateClause
  //  DMP-NEXT:     DeclRefExpr {{.*}} 'x' 'int'
  //  DMP-NEXT:   ACCSharedClause {{.*}} <implicit>
  //  DMP-NEXT:     DeclRefExpr {{.*}} 'x' 'int'
  //  DMP-NEXT:   impl: OMPTargetTeamsDirective
  //  DMP-NEXT:     OMPMapClause
  //  DMP-NEXT:       DeclRefExpr {{.*}} 'x' 'int'
  //
  //   PRT-LABEL: case CASE_PARALLEL_SCALAR_PRESENT:
  //    PRT-NEXT: {
  //    PRT-NEXT:   int x;
  //
  //  PRT-A-NEXT:   #pragma acc data copy(x){{$}}
  // PRT-AO-NEXT:   // #pragma omp target data map(tofrom: x){{$}}
  //  PRT-A-NEXT:   #pragma acc parallel no_create(x){{$}}
  // PRT-AO-NEXT:   // #pragma omp target teams map([[NO_CREATE_MT]]: x) shared(x){{$}}
  //
  //  PRT-O-NEXT:   #pragma omp target data map(tofrom: x){{$}}
  // PRT-OA-NEXT:   // #pragma acc data copy(x){{$}}
  //  PRT-O-NEXT:   #pragma omp target teams map([[NO_CREATE_MT]]: x) shared(x){{$}}
  // PRT-OA-NEXT:   // #pragma acc parallel no_create(x){{$}}
  //
  //    PRT-NEXT:   ;
  //    PRT-NEXT:   break;
  //    PRT-NEXT: }
  case CASE_PARALLEL_SCALAR_PRESENT:
  {
    int x;
    #pragma acc data copy(x)
    #pragma acc parallel no_create(x)
    x = 5;
    break;
  }

  case CASE_PARALLEL_SCALAR_ABSENT:
  {
    int x;
    int use = 0;
    #pragma acc parallel no_create(x)
    if (use) x = 5;
    break;
  }

  case CASE_PARALLEL_ARRAY_PRESENT:
  {
    int arr[3];
    #pragma acc data copy(arr)
    #pragma acc parallel no_create(arr)
    arr[0] = 5;
    break;
  }

  case CASE_PARALLEL_ARRAY_ABSENT:
  {
    int arr[3];
    int use = 0;
    #pragma acc parallel no_create(arr)
    if (use) arr[0] = 5;
    break;
  }

  case CASE_PARALLEL_SUBARRAY_PRESENT:
  {
    int all[10], same[10], beg[10], mid[10], end[10];
    #pragma acc data copy(all,same[3:6],beg[2:5],mid[1:8],end[0:5])
    #pragma acc parallel no_create(all[0:10],same[3:6],beg[2:2],mid[3:3],end[4:1])
    {
      all[0] = 1; same[3] = 1; beg[2] = 1; mid[3] = 1; end[4] = 1;
    }
    break;
  }

  case CASE_PARALLEL_SUBARRAY_DISJOINT:
  {
    int arr[4];
    int use = 0;
    #pragma acc data copy(arr[0:2])
    #pragma acc parallel no_create(arr[2:2])
    if (use) arr[2] = 1;
    break;
  }

  case CASE_PARALLEL_SUBARRAY_OVERLAP_START:
  {
    int arr[10];
    int use = 0;
    #pragma acc data copyin(arr[5:4])
    #pragma acc parallel no_create(arr[4:4])
    if (use) arr[4] = 1;
    break;
  }

  case CASE_PARALLEL_SUBARRAY_OVERLAP_END:
  {
    int arr[10];
    int use = 0;
    #pragma acc data copyin(arr[3:4])
    #pragma acc parallel no_create(arr[4:4])
    if (use) arr[4] = 1;
    break;
  }

  case CASE_PARALLEL_SUBARRAY_CONCAT2:
  {
    int arr[4];
    int use = 0;
    #pragma acc data copyout(arr[0:2])
    #pragma acc data copy(arr[2:2])
    #pragma acc parallel no_create(arr[0:4])
    if (use) arr[0] = 1;
    break;
  }

  // DMP-LABEL: CASE_PARALLEL_LOOP_SCALAR_PRESENT
  //       DMP: ACCDataDirective
  //  DMP-NEXT:   ACCCopyClause
  //  DMP-NEXT:     DeclRefExpr {{.*}} 'x' 'int'
  //  DMP-NEXT:   impl: OMPTargetDataDirective
  //  DMP-NEXT:     OMPMapClause
  //  DMP-NEXT:       DeclRefExpr {{.*}} 'x' 'int'
  //       DMP:   ACCParallelLoopDirective
  //  DMP-NEXT:     ACCNoCreateClause
  //  DMP-NEXT:       DeclRefExpr {{.*}} 'x' 'int'
  //  DMP-NEXT:     effect: ACCParallelDirective
  //  DMP-NEXT:       ACCNoCreateClause
  //  DMP-NEXT:         DeclRefExpr {{.*}} 'x' 'int'
  //  DMP-NEXT:       ACCSharedClause {{.*}} <implicit>
  //  DMP-NEXT:         DeclRefExpr {{.*}} 'x' 'int'
  //  DMP-NEXT:       impl: OMPTargetTeamsDirective
  //  DMP-NEXT:         OMPMapClause
  //  DMP-NEXT:           DeclRefExpr {{.*}} 'x' 'int'
  //       DMP:       ACCLoopDirective
  //  DMP-NEXT:         ACCIndependentClause {{.*}} <implicit>
  //  DMP-NEXT:         ACCSharedClause {{.*}} <implicit>
  //  DMP-NEXT:           DeclRefExpr {{.*}} 'x' 'int'
  //  DMP-NEXT:         ACCGangClause {{.*}} <implicit>
  //  DMP-NEXT:         impl: OMPDistributeDirective
  //  DMP-NEXT:           OMPSharedClause {{.*}} <implicit>
  //  DMP-NEXT:             DeclRefExpr {{.*}} 'x' 'int'
  //   DMP-NOT:           OMP
  //       DMP:           ForStmt
  //
  //   PRT-LABEL: case CASE_PARALLEL_LOOP_SCALAR_PRESENT
  //    PRT-NEXT: {
  //    PRT-NEXT:   int x;
  //
  //  PRT-A-NEXT:   #pragma acc data copy(x){{$}}
  // PRT-AO-NEXT:   // #pragma omp target data map(tofrom: x){{$}}
  //  PRT-A-NEXT:   #pragma acc parallel loop no_create(x){{$}}
  // PRT-AO-NEXT:   // #pragma omp target teams map([[NO_CREATE_MT]]: x) shared(x){{$}}
  // PRT-AO-NEXT:   // #pragma omp distribute{{$}}
  //
  //  PRT-O-NEXT:   #pragma omp target data map(tofrom: x){{$}}
  // PRT-OA-NEXT:   // #pragma acc data copy(x){{$}}
  //  PRT-O-NEXT:   #pragma omp target teams map([[NO_CREATE_MT]]: x) shared(x){{$}}
  //  PRT-O-NEXT:   #pragma omp distribute{{$}}
  // PRT-OA-NEXT:   // #pragma acc parallel loop no_create(x){{$}}
  //
  //    PRT-NEXT:   for ({{.*}})
  //    PRT-NEXT:     ;
  //    PRT-NEXT:   break;
  //    PRT-NEXT: }
  case CASE_PARALLEL_LOOP_SCALAR_PRESENT:
  {
    int x;
    #pragma acc data copy(x)
    #pragma acc parallel loop no_create(x)
    for (int i = 0; i < 2; ++i)
    x = 1;
    break;
  }

  case CASE_PARALLEL_LOOP_SCALAR_ABSENT:
  {
    int x;
    int use = 0;
    #pragma acc parallel loop no_create(x)
    for (int i = 0; i < 2; ++i)
    if (use) x = 1;
    break;
  }

  case CASE_CONST_PRESENT:
  {
    const int x;
    int y;
    #pragma acc data copy(x)
    #pragma acc parallel loop no_create(x)
    for (int i = 0; i < 2; ++i)
    y = x;
    break;
  }

  case CASE_CONST_ABSENT:
  {
    const int x;
    int y;
    int use = 0;
    #pragma acc parallel loop no_create(x)
    for (int i = 0; i < 2; ++i)
    if (use) y = x;
    break;
  }

  case CASE_END:
    fprintf(stderr, "unexpected CASE_END\n");
    break;
  }

  // Unfortunately, in many cases, there's no obvious way to check when
  // no_create actually doesn't allocate memory except by checking the debug
  // output of the runtime.  For example, calling acc_is_present or
  // omp_target_is_present only works from the host, so it doesn't help for
  // no_create on "acc parallel".

  // EXE-HOST-NOT: Libomptarget

  //      EXE-PRESENT: Libomptarget --> Entering data begin region
  //      EXE-PRESENT: Libomptarget --> Creating new map entry
  // EXE-PRESENT-DATA: Libomptarget --> Entering data begin region
  // EXE-PRESENT-DATA: Libomptarget --> Mapping exists {{.*}} updated RefCount=2
  // EXE-PRESENT-DATA: Libomptarget --> Entering data end region
  // EXE-PRESENT-DATA: Libomptarget --> Mapping exists {{.*}} updated RefCount=1
  //  EXE-PRESENT-PAR: Libomptarget --> Entering target region
  //  EXE-PRESENT-PAR: Libomptarget --> Launching target execution
  //      EXE-PRESENT: Libomptarget --> Entering data end region
  //      EXE-PRESENT: Libomptarget --> Mapping exists
  //      EXE-PRESENT: Libomptarget --> Removing mapping

  // EXE-NO-ALLOC-DATA: Libomptarget --> Entering data begin region
  // EXE-NO-ALLOC-DATA: Libomptarget --> Mapping does not exist
  // EXE-NO-ALLOC-DATA: Libomptarget --> Call to getOrAllocTgtPtr returned null pointer ('no_alloc' map type modifier)
  // EXE-NO-ALLOC-DATA: Libomptarget --> Entering data end region
  // EXE-NO-ALLOC-DATA: Libomptarget --> Data is not allocated on device
  //  EXE-NO-ALLOC-PAR: Libomptarget --> Entering target region
  //  EXE-NO-ALLOC-PAR: Libomptarget --> Launching target execution

  //      EXE-NO-ALLOC-DISJOINT: Libomptarget --> Entering data begin region
  //      EXE-NO-ALLOC-DISJOINT: Libomptarget --> Creating new map entry
  // EXE-NO-ALLOC-DISJOINT-DATA: Libomptarget --> Entering data begin region
  //  EXE-NO-ALLOC-DISJOINT-PAR: Libomptarget --> Entering target region
  //      EXE-NO-ALLOC-DISJOINT: Libomptarget --> Mapping does not exist
  //      EXE-NO-ALLOC-DISJOINT: Libomptarget --> Call to getOrAllocTgtPtr returned null pointer ('no_alloc' map type modifier)
  //  EXE-NO-ALLOC-DISJOINT-PAR: Libomptarget --> Launching target execution
  // EXE-NO-ALLOC-DISJOINT-DATA: Libomptarget --> Entering data end region
  //      EXE-NO-ALLOC-DISJOINT: Libomptarget --> Data is not allocated on device
  //      EXE-NO-ALLOC-DISJOINT: Libomptarget --> Entering data end region
  //      EXE-NO-ALLOC-DISJOINT: Libomptarget --> Mapping exists
  //      EXE-NO-ALLOC-DISJOINT: Libomptarget --> Removing mapping

  //      EXE-NO-ALLOC-EXTENDS: Libomptarget --> Entering data begin region
  //      EXE-NO-ALLOC-EXTENDS: Libomptarget --> Creating new map entry
  // EXE-NO-ALLOC-EXTENDS-DATA: Libomptarget --> Entering data begin region
  //  EXE-NO-ALLOC-EXTENDS-PAR: Libomptarget --> Entering target region
  //      EXE-NO-ALLOC-EXTENDS: Libomptarget --> Explicit extension of mapping is not allowed
  //      EXE-NO-ALLOC-EXTENDS: Libomptarget --> Call to getOrAllocTgtPtr returned null pointer ('no_alloc' map type modifier)
  //  EXE-NO-ALLOC-EXTENDS-PAR: Libomptarget --> Launching target execution
  // EXE-NO-ALLOC-EXTENDS-DATA: Libomptarget --> Entering data end region
  //      EXE-NO-ALLOC-EXTENDS: Libomptarget --> Data is not allocated on device
  //      EXE-NO-ALLOC-EXTENDS: Libomptarget --> Entering data end region
  //      EXE-NO-ALLOC-EXTENDS: Libomptarget --> Mapping exists
  //      EXE-NO-ALLOC-EXTENDS: Libomptarget --> Removing mapping

  //      EXE-NO-ALLOC-CONCAT2: Libomptarget --> Entering data begin region
  //      EXE-NO-ALLOC-CONCAT2: Libomptarget --> Creating new map entry
  //      EXE-NO-ALLOC-CONCAT2: Libomptarget --> Entering data begin region
  //      EXE-NO-ALLOC-CONCAT2: Libomptarget --> Creating new map entry
  // EXE-NO-ALLOC-CONCAT2-DATA: Libomptarget --> Entering data begin region
  //  EXE-NO-ALLOC-CONCAT2-PAR: Libomptarget --> Entering target region
  //      EXE-NO-ALLOC-CONCAT2: Libomptarget --> Explicit extension of mapping is not allowed
  //      EXE-NO-ALLOC-CONCAT2: Libomptarget --> Call to getOrAllocTgtPtr returned null pointer ('no_alloc' map type modifier)
  //  EXE-NO-ALLOC-CONCAT2-PAR: Libomptarget --> Launching target execution
  // EXE-NO-ALLOC-CONCAT2-DATA: Libomptarget --> Entering data end region
  //      EXE-NO-ALLOC-CONCAT2: Libomptarget --> Data is not allocated on device
  //      EXE-NO-ALLOC-CONCAT2: Libomptarget --> Entering data end region
  //      EXE-NO-ALLOC-CONCAT2: Libomptarget --> Mapping exists
  //      EXE-NO-ALLOC-CONCAT2: Libomptarget --> Removing mapping
  //      EXE-NO-ALLOC-CONCAT2: Libomptarget --> Entering data end region
  //      EXE-NO-ALLOC-CONCAT2: Libomptarget --> Mapping exists
  //      EXE-NO-ALLOC-CONCAT2: Libomptarget --> Removing mapping

  // EXE-ALLOC-DATA: Libomptarget --> Entering data begin region
  // EXE-ALLOC-DATA: Libomptarget --> Creating new map entry
  // EXE-ALLOC-DATA: Libomptarget --> Entering data end region
  // EXE-ALLOC-DATA: Libomptarget --> Mapping exists
  // EXE-ALLOC-DATA: Libomptarget --> Removing mapping
  //  EXE-ALLOC-PAR: Libomptarget --> Entering target region
  //  EXE-ALLOC-PAR: Libomptarget --> Launching target execution

  //      EXE-ALLOC-DISJOINT: Libomptarget --> Entering data begin region
  //      EXE-ALLOC-DISJOINT: Libomptarget --> Creating new map entry
  // EXE-ALLOC-DISJOINT-DATA: Libomptarget --> Entering data begin region
  //  EXE-ALLOC-DISJOINT-PAR: Libomptarget --> Entering target region
  //      EXE-ALLOC-DISJOINT: Libomptarget --> Creating new map entry
  //  EXE-ALLOC-DISJOINT-PAR: Libomptarget --> Launching target execution
  // EXE-ALLOC-DISJOINT-DATA: Libomptarget --> Entering data end region
  //      EXE-ALLOC-DISJOINT: Libomptarget --> Mapping exists
  //      EXE-ALLOC-DISJOINT: Libomptarget --> Removing mapping
  //      EXE-ALLOC-DISJOINT: Libomptarget --> Entering data end region
  //      EXE-ALLOC-DISJOINT: Libomptarget --> Mapping exists
  //      EXE-ALLOC-DISJOINT: Libomptarget --> Removing mapping

  //      EXE-ALLOC-EXTENDS: Libomptarget --> Entering data begin region
  //      EXE-ALLOC-EXTENDS: Libomptarget --> Creating new map entry
  // EXE-ALLOC-EXTENDS-DATA: Libomptarget --> Entering data begin region
  //  EXE-ALLOC-EXTENDS-PAR: Libomptarget --> Entering target region
  //      EXE-ALLOC-EXTENDS: Libomptarget --> Explicit extension of mapping is not allowed
  //      EXE-ALLOC-EXTENDS: Libomptarget --> Call to getOrAllocTgtPtr returned null pointer (device failure or illegal mapping)
  //      EXE-ALLOC-EXTENDS: Libomptarget fatal error 1: failure of target construct while offloading is mandatory

  //      EXE-ALLOC-CONCAT2: Libomptarget --> Entering data begin region
  //      EXE-ALLOC-CONCAT2: Libomptarget --> Creating new map entry
  //      EXE-ALLOC-CONCAT2: Libomptarget --> Entering data begin region
  //      EXE-ALLOC-CONCAT2: Libomptarget --> Creating new map entry
  // EXE-ALLOC-CONCAT2-DATA: Libomptarget --> Entering data begin region
  //  EXE-ALLOC-CONCAT2-PAR: Libomptarget --> Entering target region
  //      EXE-ALLOC-CONCAT2: Libomptarget --> Explicit extension of mapping is not allowed
  //      EXE-ALLOC-CONCAT2: Libomptarget --> Call to getOrAllocTgtPtr returned null pointer (device failure or illegal mapping)
  //      EXE-ALLOC-CONCAT2: Libomptarget fatal error 1: failure of target construct while offloading is mandatory

  return 0;
}
