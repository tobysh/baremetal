---
title: The Context Switch
summary: Write the assembly that swaps one stack for another: save callee-saved registers, change rsp, restore, return. Then build a fresh task's first stack frame.
minutes: 60
difficulty: hard
tags: scheduling, assembly, processes
id: osdev/context-switch
mode: harness
---
The scheduler said *which* task runs next. Now you have to make the CPU **become** that task. This is the most famous trick in operating systems, and it is only a handful of instructions.

## The trick: a task is a stack

A task's whole state is its registers plus its stack. If you save the registers and move the stack pointer `rsp` to another task's stack, then restore *its* registers and `ret`, execution continues in the other task exactly where it was. The `ret` pops the return address that task left on its own stack the last time it switched away.

So a switch is a function that **returns on a different stack** than the one it was called on:

```c
void ctx_switch(void **save_sp, void *new_sp);
```

It stores the old `rsp` in `*save_sp` and loads `new_sp`. From the caller's point of view it simply "takes a long time to return", while other tasks run.

## What must be saved

The System V calling convention divides registers into two groups:

- **Caller-saved** (`rax`, `rcx`, `rdx`, `rsi`, `rdi`, `r8`–`r11`): a function may destroy them, so whoever called `ctx_switch` already assumes they are gone. No need to save these.
- **Callee-saved** (`rbx`, `rbp`, `r12`, `r13`, `r14`, `r15`): a function must restore them before returning. Since `ctx_switch` is a function whose "return" is really a different task's return, it must hand each task *its own* callee-saved registers back. `rsp` is handled by the switch itself; `rip` is the return address.

So `ctx_switch` is:

```
push the six callee-saved registers      ; onto the old stack
mov  [rdi], rsp                          ; remember where the old stack ended
mov  rsp, rsi                            ; switch stacks!
pop  the six registers                   ; from the new stack, in reverse
ret                                      ; resume the new task
```

The arguments follow the System V convention: the first argument is in `rdi`, the second in `rsi`. In C, `rdi` is `save_sp`, so `[rdi]` stores through the pointer.

## Writing it in a C file

C can't express this, but GCC lets you put assembly at the top level. The starter sets it up for you:

```c
__asm__(
    ".text\n"
    ".intel_syntax noprefix\n"
    ".globl ctx_switch\n"
    "ctx_switch:\n"
    "    ret\n"
    ".att_syntax prefix\n");
```

(GCC writes AT&T syntax by default, so the block switches to Intel syntax to match the rest of this course, and back afterwards.) You replace the single `ret` with the real body.

## Starting a new task

A brand-new task has never run, so it has no saved state to restore. We *fake* it: build the stack frame that `ctx_switch` would have left. The first time anyone switches to it, `ctx_switch` pops six values and `ret`s into the task's entry function.

Lay out the stack from the top, growing downward:

```
stack_top  ->
  -8   address of task_finished   (if entry ever returns, it "returns" here)
  -16  address of entry           (ctx_switch's ret pops this)
  -24  rbx = 0                    \
  -32  rbp = 0                     |  six slots that ctx_switch pops
  -40  r12 = 0                     |
  -48  r13 = 0                     |
  -56  r14 = 0                     |
  -64  r15 = 0                    /     <- the value task_stack_init returns
```

`task_stack_init` returns the pointer to the lowest slot, `stack_top - 64`; that is the `new_sp` for the first `ctx_switch`.

### Alignment

The ABI says that at the first instruction of a function, `rsp + 8` is a multiple of 16 (the call pushed an 8-byte return address on top of an aligned stack). When `ctx_switch` executes its `ret`, `rsp` ends up `stack_top - 8`, which is exactly what a real call would have left. That holds only if `stack_top` is 16-byte aligned; the tests guarantee it. Misalignment crashes much later, in some instruction that needs aligned data. Those are the nastiest bugs.

## A worked trace

Suppose `main` is running on the boot stack and calls `ctx_switch(&main_sp, new_sp)`, where `new_sp = top - 64` is the value `task_stack_init` returned for a fresh task whose `entry` is `task`. Follow every instruction.

**On `main`'s stack, at the call.**

```
call ctx_switch      pushes the return address into main (R_main)
                     main stack:  ... [R_main] <- rsp
push rbp .. r15      six more pushes:
                     main stack:  ... [R_main][rbp][rbx][r12][r13][r14][r15] <- rsp
mov [rdi], rsp       main_sp = that rsp. This is the only place main's state is remembered.
mov rsp, rsi         rsp = top - 64:  we are now standing on the new task's stack
```

**On the new task's stack.** The six zero slots and the two addresses built by `task_stack_init` are waiting:

```
 top-8   [ task_finished ]
 top-16  [ task          ]
 top-24  [ 0 ]   <- rbx   (pop order: r15 first, at top-64)
 ...
 top-64  [ 0 ]   <- rsp now
pop r15 ... pop rbp  six pops: rsp = top - 16, and all six registers are zero
ret                  pops `task` into rip: rsp = top - 8. The CPU is now running task().
```

At that moment `rsp = top - 8`. A normal function call leaves `rsp` 8 bytes below a 16-byte boundary (because `call` pushed 8), and `top` is 16-aligned, so `top - 8` is exactly that. The new task thinks it was called by someone: its return address is `task_finished`.

**Switching back.** When `task` later calls `ctx_switch(&task_sp, main_sp)`, the same sequence runs in the other direction: it pushes the task's registers on the task's stack, stores `task_sp`, sets `rsp = main_sp`, pops `main`'s six saved registers (which were pushed at the very start of this trace) and `ret` pops `R_main`. To `main`, `ctx_switch(...)` simply returns, after however long that took.

Compare the two pictures and you can see the symmetry: a *new* task begins by "returning" into its entry function, a *resumed* task begins by "returning" from its own earlier call to `ctx_switch`. The fake frame from `task_stack_init` makes a task that never ran look like one that did.

## Why only six registers

`rax`, `rcx`, `rdx`, `rsi`, `rdi` and `r8` to `r11` are caller-saved: any function call may destroy them, so the C compiler never keeps a live value in one across a call. `ctx_switch` is a function call, so the compiler already saved what it needed. The six callee-saved registers are different: the compiler *does* keep live values in them across calls, so they must come back exactly as they were. `rsp` and `rip` are the switch's own business. The flags register needs no saving either. (`rflags`' direction flag is required to be clear at function boundaries, and the interrupt flag is managed by whoever calls.) The SSE and x87 state isn't touched by this kernel, which is compiled without SSE. A kernel that uses floating point must save that too, which is why many kernels forbid it in the kernel proper.

## Debugging checklist

- **Triple fault on the first switch:** the frame doesn't match the pops. Count: six registers plus the entry address must be seven slots, and the guard address makes eight. `task_stack_init` must return `top - 64`.
- **The task runs, but crashes on its first call or push:** `rsp` is misaligned. `top` must be a multiple of 16 and the entry slot at `top - 16`.
- **`main` continues with wrong values after the switch back:** a push and its pop are mismatched, or the pops aren't in the exact reverse order of the pushes. If you push `rbp, rbx, r12..r15` you pop `r15..r12, rbx, rbp`.
- **The old stack pointer is lost:** you moved `rsp` before storing it. The order is *pushes, store, load, pops, ret*.
- **`undefined reference to ctx_switch`:** the `.globl` line is missing, or the assembly block ended in the wrong syntax mode. It must end with `.att_syntax prefix` so the compiler's own output still assembles.

## Your turn

Fill in the body of `ctx_switch` and write `task_stack_init`. The tests create real tasks with their own stacks and bounce between them thousands of times.
