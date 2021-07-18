// RUN: %clang -S -emit-llvm %s -o - \
// RUN:   | opt -load  %shlibdir/libProfiler%shlibext -legacy-profiler -S -o %t.ll
// RUN: %clang %t.ll -o %t.bin
// RUN: %t.bin | FileCheck --check-prefix=CHECK-UNPROFILED %s

// CHECK-UNPROFILED-NOT: Profiling basic block
// CHECK: is at index

// RUN: %clang -shared -fPIC %S/ProfilerRoutine.c -o \
// RUN:     %T/libProfilerRoutine%shlibext

// RUN: PROFILER_VERBOSE_ENV=true PROFILER_THRESHOLD_ENV=1 \
// RUN: LD_PRELOAD=%shlibdir/libProfilerRuntime%shlibext %t.bin | \ 
// RUN: FileCheck --check-prefix=CHECK-PROFILER-FIRST-EXE %s

// CHECK-PROFILER-FIRST-EXE: Profiling basic block
// CHECK-PROFILED-FIRST-EXE: was executed more than

// RUN: PROFILER_VERBOSE_ENV=true \
// RUN: LD_PRELOAD=%shlibdir/libProfilerRuntime%shlibext %t.bin | \
// RUN: FileCheck --check-prefix=CHECK-PROFILED-SECOND-EXE %s

// CHECK-PROFILED-SECOND-EXE: Profiling basic block
// CHECK-PROFILED-SECOND-EXE-NOT: was executed more than

#include <stdio.h>

int bsearch(int *a, int n, int x) {
  int i = 0, j = n - 1;
  while (i <= j) {
    int k = i + ((j - i) / 2);
    if (a[k] == x) {
      return k;
    } else if (a[k] < x) {
      i = k + 1;
    } else {
      j = k - 1;
    }
  }
  return -1;
}

int main() {

  int a[] = {59, 15, 64, 44, 5,   60, 90, 68, 98, 61, 14, 26, 45, 71, 55,
             97, 52, 86, 3,  70,  11, 40, 43, 32, 94, 81, 74, 85, 82, 89,
             46, 47, 67, 65, 79,  72, 22, 76, 75, 13, 93, 51, 31, 6,  91,
             7,  57, 83, 27, 73,  16, 66, 58, 24, 2,  8,  34, 9,  92, 25,
             17, 99, 54, 30, 29,  88, 96, 4,  49, 48, 80, 33, 50, 95, 62,
             41, 77, 12, 69, 53,  38, 37, 19, 78, 21, 63, 87, 36, 1,  20,
             28, 10, 23, 42, 100, 18, 56, 39, 35, 84};
  int n = sizeof a / sizeof a[0];
  int x = 84;
  int i = bsearch(a, n, x);
  printf("%d is at index %d\n", x, i);
  return 0;
}
