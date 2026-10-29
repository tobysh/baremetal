---
title: Calling libc from Assembly
summary: Your assembly can call snprintf and strlen exactly as C does. Learn variadic calls, string constants in .rodata, and how results survive between calls.
minutes: 30
difficulty: medium
tags: libc, calling-convention, strings
id: asm-systems/libc-calls
mode: harness
---
The hidden test program is linked with the C library, so every libc function is available to you by name. Calling one is the same recipe as calling your own function: arguments in `rdi, rsi, rdx, rcx, r8, r9`, an aligned stack, `call name`, result in `rax`. The linker routes the call through the procedure linkage table for you.

## String constants

Put read-only data in `.rodata` and load its address with a RIP-relative `lea`:

```asm
.section .rodata
.Lfmt: .asciz "%ld,%ld"

.text
    lea rdx, [rip + .Lfmt]      # address of the string
```

`.asciz` appends the NUL terminator, `.ascii` does not. Switch back to `.text` before more code.

## Variadic functions: `al` matters

`printf`, `snprintf` and friends take a variable number of arguments. The ABI adds one rule: before the call, **`al` must hold an upper bound on the number of vector registers used for arguments**. With only integer arguments that is zero:

```asm
xor eax, eax            # al = 0
call snprintf
```

Forget it and `al` may contain leftover garbage; the function then saves `xmm` registers it never needed, and sometimes crashes on a misaligned save.

## Shuffling arguments into place

`int snprintf(char *buf, size_t n, const char *fmt, ...)` wants `buf` in `rdi`, `n` in `rsi`, the format in `rdx`, then the first vararg in `rcx` and the second in `r8`. But your function `format_pair(buf, n, a, b)` receives `a` in `rdx` and `b` in `rcx`. The order of the moves matters, or you overwrite a value before copying it:

```asm
mov r8, rcx             # b -> second vararg  (first, because rcx is about to change)
mov rcx, rdx            # a -> first vararg
lea rdx, [rip + .Lfmt]  # now rdx is free for the format
```

This is a general technique: draw a table of "where it is" versus "where it must go" and move the ones whose destinations are unoccupied first.

## Keeping values across calls

`joined_length(a, b)` needs `strlen(a) + strlen(b)`. After the first `strlen`, the register holding `b` is gone. Keep `b` and the first length in stack slots (`sub rsp, 24` again gives alignment plus two slots) or in callee-saved registers. Either way, the alignment rule from earlier lessons applies to every libc call too.

## Tail calls

When the last thing a function does is call another function and return its result unchanged, you can replace `call f` / `ret` by `jmp f`, as long as the stack is in the state it was on entry. `format_pair` can use this: after rearranging arguments (and with no stack adjustment made), `jmp snprintf` returns `snprintf`'s result directly to your caller. Compilers do this at `-O2` all the time. Using `call` with a `sub rsp, 8` is equally correct.
