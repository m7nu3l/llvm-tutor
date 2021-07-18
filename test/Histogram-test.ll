; RUN: opt -load %shlibdir/libHistogram%shlibext -legacy-histogram -histogram-output-file=%T/histogram.out -S %s

; RUN: FileCheck %s < %T/histogram.out
; CHECK: 2
; CHECK: 3
; CHECK: 8
; CHECK: 3
; CHECK: 2
; CHECK: 3
; CHECK: 2

define i32 @binsearch(i32* readonly %0, i32 %1, i32 %2, i32 %3)  {
  %5 = icmp slt i32 %2, %1
  br i1 %5, label %25, label %6

6:                                                ; preds = %4, %22
  %7 = phi i32 [ %10, %22 ], [ %2, %4 ]
  %8 = phi i32 [ %23, %22 ], [ %1, %4 ]
  br label %9

9:                                                ; preds = %6, %17
  %10 = phi i32 [ %7, %6 ], [ %18, %17 ]
  %11 = add nsw i32 %10, %8
  %12 = sdiv i32 %11, 2
  %13 = sext i32 %12 to i64
  %14 = getelementptr inbounds i32, i32* %0, i64 %13
  %15 = load i32, i32* %14, align 4
  %16 = icmp sgt i32 %15, %3
  br i1 %16, label %17, label %20

17:                                               ; preds = %9
  %18 = add nsw i32 %12, -1
  %19 = icmp sgt i32 %12, %8
  br i1 %19, label %9, label %25

20:                                               ; preds = %9
  %21 = icmp slt i32 %15, %3
  br i1 %21, label %22, label %25

22:                                               ; preds = %20
  %23 = add nsw i32 %12, 1
  %24 = icmp sgt i32 %10, %12
  br i1 %24, label %6, label %25

25:                                               ; preds = %20, %22, %17, %4
  %26 = phi i32 [ -1, %4 ], [ -1, %17 ], [ -1, %22 ], [ %12, %20 ]
  ret i32 %26
}
