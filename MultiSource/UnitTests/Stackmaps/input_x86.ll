@test_input = global i64 9833440827789222417 ; 0x8877665544332211

declare void @sm_inspect()
declare void @llvm.experimental.patchpoint.void(i64, i32, i8*, i32, ...)

define void @integer_tests(i64 %test_input) noinline {
  %fptr = bitcast void()* @sm_inspect to i8*

  ; Test constant integers.
  call void (i64, i32, i8*, i32, ...) @llvm.experimental.patchpoint.void(i64 0,
    i32 16, i8* %fptr, i32 0, i8 50, i16 51, i32 52, i64 53)

  ; Test integers in registers.
  ; The addition operations force each value into a distinct register.
  %test_input_plus1 = add i64 %test_input, 1
  %test_input_plus2 = add i64 %test_input, 2
  %test_input_plus3 = add i64 %test_input, 3
  %test_input_plus4 = add i64 %test_input, 3
  %arg8 = trunc i64 %test_input_plus1 to i8
  %arg16 = trunc i64 %test_input_plus2 to i16
  %arg32 = trunc i64 %test_input_plus3 to i32
  call void (i64, i32, i8*, i32, ...) @llvm.experimental.patchpoint.void(i64 1, i32
  16, i8* %fptr, i32 0, i8 %arg8, i16 %arg16, i32 %arg32, i64 %test_input)

  ret void
}

; Test integers on the stack.
define void @integer_tests2(i64 %test_input) noinline {
  %fptr = bitcast void()* @sm_inspect to i8*

  %test_input_plus1 = add i64 %test_input, 1
  %test_input_plus2 = add i64 %test_input, 2
  %test_input_plus3 = add i64 %test_input, 3
  %test_input_plus4 = add i64 %test_input, 3
  %arg8 = trunc i64 %test_input_plus1 to i8
  %arg16 = trunc i64 %test_input_plus2 to i16
  %arg32 = trunc i64 %test_input_plus3 to i32

  ; By clobbering all of the general puprpose registers, we force this frame's
  ; local variables to be allocated on the stack.
  call void asm sideeffect "nop; nop; nop", "~{rax},~{rdx},~{rcx},~{rbx},~{rsi},~{rdi},~{rbp},~{rsp},~{r8},~{r9},~{r10},~{r11},~{r12},~{r13},~{r14},~{r15}"()

  call void (i64, i32, i8*, i32, ...) @llvm.experimental.patchpoint.void(i64 2, i32
  16, i8* %fptr, i32 0, i8 %arg8, i16 %arg16, i32 %arg32, i64 %test_input)

  ret void
}

define void @run_tests(i64* %arg) noinline {
  %test_input = load i64, i64* %arg
  call void @integer_tests(i64 %test_input)
  call void @integer_tests2(i64 %test_input)
  ret void
}
