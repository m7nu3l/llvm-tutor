// RUN: %clang -S -emit-llvm %s -o - \
// RUN:   | opt -load  %shlibdir/libProfiler%shlibext -legacy-profiler -S -o %t.ll
// RUN: %clang %t.ll -o %t.bin
// RUN: %t.bin | FileCheck --check-prefix=CHECK-UNPROFILED %s

// CHECK-UNPROFILED-NOT: BB_EXEC
// CHECK: is at index

// RUN: %clang -shared -fPIC %S/ProfilerRoutine.c -o %T/libProfilerRoutine%shlibext 
// RUN: LD_PRELOAD=%T/libProfilerRoutine%shlibext %t.bin | FileCheck --check-prefix=CHECK-PROFILED %s 

// CHECK-PROFILED: BB_EXEC

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
  int a[] = {-31, 0, 1, 2, 2, 4, 65, 83, 99, 782};
  int n = sizeof a / sizeof a[0];
  int x = 2;
  int i = bsearch(a, n, x);
  printf("%d is at index %d\n", x, i);
  return 0;
}
