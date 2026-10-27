---
title: The Stack and Recursion
summary: Use the stack, call and ret to write a recursive function, save callee-saved registers safely, and keep rsp 16-byte aligned.
minutes: 30
difficulty: hard
id: asm-first-steps/factorial
mode: harness
---
A function that calls another function (even itself) has to deal with the **stack**. This is the lesson where assembly stops being "just arithmetic".

## The stack

The stack is a region of memory that grows toward lower addresses. `rsp` always points at its top. Two instructions use it:

```asm
push rbx      # rsp -= 8, then store rbx at [rsp]
pop rbx       # load rbx from [rsp], then rsp += 8
```

## call and ret

`call f` pushes the **return address** (the address of the next instruction) and jumps to `f`. `ret` pops that address into the instruction pointer. So on entry to any function, `[rsp]` holds the return address, and your function must leave `rsp` exactly as it found it before `ret`: every `push` needs a matching `pop`.

### Drawing the stack

Addresses grow upward on paper, but the stack grows *downward*, so the picture is usually drawn with high addresses at the top and the newest data at the bottom. Here is what the stack holds just after a caller executes `call my_function`:

```
higher addresses
+--------------------------+
| caller's local data      |
+--------------------------+
| return address           |  <- rsp (points at the top item)
+--------------------------+
lower addresses
```

If the function now does `push rbx`, the old `rbx` goes below it and `rsp` moves down by 8. `pop rbx` reverses that. Because the stack is last in, first out, the pushes and pops must nest cleanly, exactly like matching brackets. The stack is also just memory: you can address it directly (`mov rax, [rsp + 8]` reads the item one slot down from the top) and use `sub rsp, 32` to reserve 32 bytes for local variables, then `add rsp, 32` to give them back.

## Alignment

The ABI promises that `rsp` is a multiple of 16 *at the moment of a `call`*. The `call` itself pushes 8 bytes, so on entry to a function `rsp` is **8 modulo 16**. If your function calls something, you must adjust the stack by another 8 (mod 16) first. One `push` does exactly that:

```
on entry:        rsp = 16k + 8    (return address just pushed)
after push rbx:  rsp = 16k        (aligned: safe to call)
```

Misaligned calls often crash later, in code like `printf`, using instructions that need 16-byte alignment.

## Callee-saved registers

Registers `rbx`, `rbp` and `r12` to `r15` belong to the **caller**. If you want to use one, push its old value on entry and pop it before `ret`. They are the right place to keep a value that must survive a `call`, because the called function is obliged to preserve them. Caller-saved registers (`rax`, `rcx`, `rdx`, `rsi`, `rdi`, `r8` to `r11`) may be wrecked by any call.

## Recursive factorial

`n! = n * (n-1)!`, and `0! = 1! = 1`. In assembly:

```asm
factorial:
    cmp rdi, 1
    jbe .Lbase              # n <= 1: answer is 1
    push rbx                # save caller's rbx, and align the stack
    mov rbx, rdi            # keep n across the call
    lea rdi, [rdi - 1]      # argument = n - 1
    call factorial          # rax = (n-1)!
    imul rax, rbx           # rax = (n-1)! * n
    pop rbx                 # restore caller's rbx
    ret
.Lbase:
    mov eax, 1
    ret
```

Walk through `factorial(3)`: it stores 3 in `rbx`, calls `factorial(2)`, which stores 2 in `rbx` (after saving the 3 on the stack) and calls `factorial(1)`, which returns 1. Then each level multiplies by its own `n` and restores the previous `rbx` on the way out.

### What the stack looks like at the deepest point

When `factorial(3)` has made its way down to the base case, three calls are in progress. Each non-base call has pushed its own copy of `rbx` and been given a return address by `call`:

```
+---------------------------------+  <- older
| return address into the caller  |
| saved rbx (caller's value)      |   frame of factorial(3), where rbx = 3
| return address into factorial(3)|
| saved rbx (value 3)             |   frame of factorial(2), where rbx = 2
| return address into factorial(2)|   factorial(1) hit the base case,
+---------------------------------+  <- rsp  pushed nothing, and returns now
```

As each call returns, `rax` holds the factorial of one less. The caller multiplies it by its own `rbx` (which is the `n` it was called with), then `pop rbx` restores the value of its caller. This is why `rbx` is the right place to keep `n`: a register survives the `call`, and the saved copy on the stack means the register can be reused one level down without losing anything. Each level of recursion costs 16 bytes of stack (the return address and the saved register), so very deep recursion eventually runs out of stack. Here the recursion is at most 20 levels deep, which is nothing.

### Checking alignment yourself

You can verify the alignment rule by counting. Suppose the code that calls `factorial` had `rsp = 0x7FFC0010` (a multiple of 16). `call` pushes 8 bytes, so on entry `rsp = 0x7FFC0008`. `push rbx` makes it `0x7FFC0000`, again a multiple of 16, ready for the next `call`. The recursive call pushes a return address, and the pattern repeats one level down. The rule is the same at every level, which is why one `push` at the top of the function is all you need.

`imul a, b` is signed multiplication of `a` by `b`; for this problem the low 64 bits are the same signed or unsigned.

## Common mistakes

- Using `rdi` after the `call`: it has been overwritten by the recursive call. Save what you need in a callee-saved register or on the stack.
- Unbalanced `push`/`pop`, especially when you `ret` early from a branch. The base case above never pushed, so it never pops.
- A misaligned stack at the `call`.
- Forgetting to restore `rbx`: the tests catch this.

Now implement `factorial`.
