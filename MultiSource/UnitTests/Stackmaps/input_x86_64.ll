declare void @sm_set_inspect_sizes(i32, ...)
declare void @sm_inspect()
declare void @llvm.experimental.patchpoint.void(i64, i32, i8*, i32, ...)

define void @integer_tests(i64 %test_input) noinline {
  %fptr = bitcast void()* @sm_inspect to i8*

  ; Test constant integers.
  call void (i32, ...) @sm_set_inspect_sizes(i32 4, i64 1, i64 2, i64 3, i64 4);
  call void(i64, i32, i8*, i32, ...) @llvm.experimental.patchpoint.void(
    i64 0, i32 16, i8* %fptr, i32 0,
    i8 50, i16 51, i32 52, i64 53)
    ret void
}

; Test integers in registers.
define void @integer_tests2(i64 %test_input) noinline {
  %fptr = bitcast void()* @sm_inspect to i8*
  %test_input_plus1 = add i64 %test_input, 1
  %test_input_plus2 = add i64 %test_input, 2
  %test_input_plus3 = add i64 %test_input, 3
  %arg8 = trunc i64 %test_input_plus1 to i8
  %arg16 = trunc i64 %test_input_plus2 to i16
  %arg32 = trunc i64 %test_input_plus3 to i32
  call void (i32, ...) @sm_set_inspect_sizes(i32 4, i64 1, i64 2, i64 4, i64 8);
  call void(i64, i32, i8*, i32, ...) @llvm.experimental.patchpoint.void(
    i64 1, i32 16, i8* %fptr, i32 0,
    i8 %arg8, i16 %arg16, i32 %arg32, i64 %test_input)

  ret void
}

; Test integers on the stack.
define void @integer_tests3(i64 %test_input) noinline {
  %fptr = bitcast void()* @sm_inspect to i8*

  %test_input_plus1 = add i64 %test_input, 1
  %test_input_plus2 = add i64 %test_input, 2
  %test_input_plus3 = add i64 %test_input, 3
  %arg8 = trunc i64 %test_input_plus1 to i8
  %arg16 = trunc i64 %test_input_plus2 to i16
  %arg32 = trunc i64 %test_input_plus3 to i32

  ; Force local variables to be spilled.
  call void asm sideeffect "", "~{rax},~{rdx},~{rcx},~{rbx},~{rsi},~{rdi},~{rbp},~{rsp},~{r8},~{r9},~{r10},~{r11},~{r12},~{r13},~{r14},~{r15}"()

  call void (i32, ...) @sm_set_inspect_sizes(i32 4, i64 1, i64 2, i64 4, i64 8);
  call void(i64, i32, i8*, i32, ...) @llvm.experimental.patchpoint.void(
    i64 2, i32 16, i8* %fptr, i32 0,
    i8 %arg8, i16 %arg16, i32 %arg32, i64 %test_input)

  ret void
}

define void @frame_tests() noinline {
  %fptr = bitcast void()* @sm_inspect to i8*

  %a8 = alloca i8
  %a16 = alloca i16
  %a32 = alloca i32
  %a64 = alloca i64
  store i8 170, i8* %a8                     ; 0xaa
  store i16 48059, i16* %a16                ; 0xbbbb
  store i32 3435973836, i32* %a32           ; 0xcccccccc
  store i64 15987178197214944733, i64* %a64 ; 0xdddddddddddddddd

  call void (i32, ...) @sm_set_inspect_sizes(i32 4, i64 1, i64 2, i64 4, i64 8);
  call void(i64, i32, i8*, i32, ...) @llvm.experimental.patchpoint.void(
    i64 3, i32 16, i8* %fptr, i32 0, i8* %a8, i16* %a16, i32* %a32, i64* %a64)

  ret void
}

define void @run_tests() noinline {
  ; 0x8877665544332211
  %in1 = call i64 asm sideeffect "nop", "=r,0"(i64 9833440827789222417)
  call void @integer_tests(i64 %in1)

  ; 0x1122334455667788
  %in2 = call i64 asm sideeffect "", "=r,0"(i64 4822678189205111)
  call void @integer_tests2(i64 %in2)
  call void @integer_tests3(i64 %in2)

  call void @frame_tests()

  ret void
}
