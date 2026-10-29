---
title: Stack Frames and Local Variables
summary: Build a proper frame with push rbp, reserve local storage with sub rsp, and tear it down with leave. Use a scratch buffer on the stack to format a number.
minutes: 30
difficulty: medium
tags: stack, calling-convention, strings
id: asm-systems/stack-frames
mode: harness
---
So far you kept values in registers. When you need an array or a buffer, or more variables than there are registers, you reserve space on the **stack**. The region a function owns, from its return address down to its lowest local, is its **stack frame**.

## The classic frame

```asm
f:
    push rbp            # save caller's frame pointer
    mov  rbp, rsp       # rbp now marks the top of this frame
    sub  rsp, 32        # reserve 32 bytes of locals
    ...
    leave               # same as: mov rsp, rbp ; pop rbp
    ret
```

Locals are addressed relative to `rbp`: `[rbp - 8]`, `[rbp - 32]` and so on. Stack memory grows **downward**, so a local buffer of 32 bytes occupies `[rbp - 32, rbp)` and its first byte is at `rbp - 32`.

`rbp` is callee-saved, so the `push rbp` / `leave` pair both preserves it and, because `leave` resets `rsp` from `rbp`, guarantees that you cannot forget to give back the locals even if you change `rsp` mid-function.

### Keeping the alignment

On entry `rsp` is 8 mod 16. `push rbp` makes it 0 mod 16. So reserve a **multiple of 16** with `sub rsp` and the frame stays aligned for any `call` you make. Always round locals up to 16.

### The red zone

Leaf functions may skip the `sub rsp` and use the 128 bytes below `rsp` freely. That area is called the **red zone**; signal handlers and the kernel promise not to touch it. It disappears as soon as you call anything, so for this track build real frames.

## Why compilers use a frame pointer

The frame pointer gives every local a fixed address even while `rsp` moves with `push` and `pop`. Debuggers use the chain of saved `rbp` values to print a backtrace. Optimised code often omits it (`-fomit-frame-pointer`) and addresses locals from `rsp` instead.

## A digit buffer

You will convert an unsigned number to decimal text. Division gives the *last* digit first (`v % 10`), but text must be written first-digit-first. The standard solution is to produce the digits backwards into a scratch buffer on the stack, then copy them forward into the caller's buffer:

```c
size_t to_decimal(char *buf, unsigned long v) {
    char tmp[32];
    char *p = tmp + 32;                 // one past the end
    do { *--p = '0' + v % 10; v /= 10; } while (v);
    size_t len = (tmp + 32) - p;
    for (size_t i = 0; i < len; i++) buf[i] = p[i];
    buf[len] = 0;
    return len;
}
```

The 64-bit `div rcx` divides `rdx:rax` by `rcx`; **set `rdx` to 0 first** (`xor edx, edx`). The quotient lands in `rax` and the remainder in `rdx`. Note `div` clobbers `rdx`, which is also where your second argument arrived, so move it out first.

The maximum 64-bit value has 20 digits, so 32 bytes is plenty.
