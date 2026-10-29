---
title: Calling Function Pointers
summary: An indirect call jumps through a register. Write map and fold over arrays with callbacks, and keep your loop state safe in callee-saved registers.
minutes: 40
difficulty: hard
tags: function-pointers, calling-convention, stack
id: asm-intermediate/function-pointers
mode: harness
---
In C a callback is a parameter like `long (*f)(long)`. In assembly it is simply an address in a register, and you call it with an **indirect call**:

```asm
call r13                # call the function whose address is in r13
call [rbx + 8]          # or through a pointer stored in memory
```

The callee follows the System V rules: arguments in `rdi`, `rsi`, ..., result in `rax`, and **every caller-saved register may be destroyed**: `rax rcx rdx rsi rdi r8-r11` (and all `xmm`). Anything your loop needs afterwards, the cursor, the remaining count, the function pointer itself, must therefore live in **callee-saved** registers (`rbx rbp r12-r15`) or on the stack. That in turn makes *you* responsible for saving the caller's values of those registers.

## Recipe for a loop that calls out

1. `push` the callee-saved registers you will use.
2. Keep `rsp` 16-byte aligned at the `call`. On entry `rsp` is 8 mod 16 (the return address), so an **odd** number of 8-byte pushes aligns it; with an even number add `sub rsp, 8`.
3. Move your arguments into those registers right away, before the first `call`.
4. At the end, `pop` in reverse order.

```asm
map_inplace:                # (long *a, size_t n, long (*f)(long))
    push rbx
    push r12
    push r13                # three pushes: aligned
    mov rbx, rdi            # cursor
    mov r12, rsi            # remaining
    mov r13, rdx            # f
.Lloop:
    test r12, r12
    jz .Ldone
    mov rdi, [rbx]
    call r13
    mov [rbx], rax
    add rbx, 8
    dec r12
    jmp .Lloop
```

The tests here use callbacks that deliberately scribble over every caller-saved register, and check that your function leaves `rbx`, `rbp` and `r12`-`r15` as it found them.
