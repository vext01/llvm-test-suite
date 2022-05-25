@test_input = global i64 9833440827789222417 ; 0x8877665544332211

define void @run_tests(i64* %test_input) noinline {
  %arg64 = load i64, i64* %test_input

  ; Test constants.
  call void (i64, i32, ...) @llvm.experimental.stackmap(i64 0, i32 0,
    i8 50, i16 51, i32 52, i64 53)
  call void @sm_inspect()

  ; Test non-constants.
  ; This addition forces each value into a distinct register.
  %arg64_plus1 = add i64 %arg64, 1
  %arg64_plus2 = add i64 %arg64, 2
  %arg64_plus3 = add i64 %arg64, 3
  %arg8 = trunc i64 %arg64_plus1 to i8
  %arg16 = trunc i64 %arg64_plus2 to i16
  %arg32 = trunc i64 %arg64_plus3 to i32
  call void (i64, i32, ...) @llvm.experimental.stackmap(i64 1, i32 0,
    i8 %arg8, i16 %arg16, i32 %arg32, i64 %arg64)
  notail call void @sm_inspect()

  ret void
}

declare void @llvm.experimental.stackmap(i64, i32, ...)
declare void @sm_inspect()
