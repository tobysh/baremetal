---
title: System Calls and Distrusting Pointers
summary: Dispatch system calls through a table, return negative errno codes, and validate every user pointer without overflow before the kernel touches it.
minutes: 50
difficulty: medium
tags: syscalls, security
id: osdev/syscalls
mode: harness
---
A user program can't do much by itself. It can't touch a disk, a screen or another process. To get anything done it asks the kernel, using a **system call**. The CPU provides a controlled doorway (the `syscall` instruction or a software interrupt) that switches into kernel mode and starts running kernel code at a known address. The program passes a **number** saying what it wants, plus up to six arguments in registers.

## The syscall table

The kernel has one function per call. A table indexed by number holds their addresses, so the dispatcher is tiny:

```c
typedef int64_t (*syscall_fn)(uint64_t a1, uint64_t a2, uint64_t a3);
static syscall_fn syscall_table[64];

int64_t syscall_dispatch(uint64_t num, uint64_t a1, uint64_t a2, uint64_t a3) {
    if (num >= 64 || !syscall_table[num]) return -ENOSYS;
    return syscall_table[num](a1, a2, a3);
}
```

The check `num >= 64` is not optional. **The number comes from user code and cannot be trusted.** Without it, a program could pick `num = 1000000` and make the kernel read a function pointer from, and then jump to, memory the program chose.

We use Linux's numbers: `0` is `read`, `1` is `write`, `39` is `getpid`, `60` is `exit`.

## Errors are negative numbers

Success returns a value of 0 or more. Failure returns **minus an error code** (`-EBADF`, `-EFAULT`, ...). Here they are (the values are Linux's):

| Name | Value | Meaning |
|------|-------|---------|
| `EBADF` | 9 | bad file descriptor |
| `EFAULT` | 14 | bad address |
| `EINVAL` | 22 | invalid argument |
| `ENOSYS` | 38 | no such system call |

## The golden rule: never trust a user pointer

`write(1, buf, len)` hands the kernel a `buf` and a `len`. A malicious (or just buggy) program can pass:

- a pointer into **kernel memory**, hoping the kernel copies secrets out for it;
- a range that **starts in user space and ends in kernel space**;
- a length so huge that `ptr + len` **wraps around** past 2^64 and lands back at a small number. A check like `ptr + len <= USER_TOP` is fooled by this.

User space is the lower half of the address space, `0` to `USER_TOP = 0x00007FFFFFFFFFFF`. A range of `len` bytes starting at `ptr` lies inside it only when `len == 0`, or `ptr <= USER_TOP` and `len - 1 <= USER_TOP - ptr`. Both sides of that comparison stay in range, so it cannot wrap.

The kernel must run `user_range_ok` **before the first byte is read or written**. And then it copies the data into kernel memory first (`copy_from_user`) rather than working on the user's buffer in place. A second thread could change the user buffer between the check and the use. (A real kernel also installs a page-fault fixup so that an *unmapped* user address returns `-EFAULT` instead of crashing the kernel. We keep that part conceptual here.)

## Your calls

- `write(fd, buf, len)`: fd 1 or 2 only. Copy at most **64 bytes at a time** through a small kernel buffer and send each chunk to `console_write`, so a huge `len` never needs a huge kernel buffer.
- `read(fd, buf, len)`: fd 0 only. Hand out bytes of the pending keyboard input with `copy_to_user`.
- `getpid()`, `exit(code)`.

## How a real system call gets into the kernel

Your `syscall_dispatch` is the middle of the journey. Here's the whole trip on x86-64 Linux:

1. The program puts the **number** in `rax` and the arguments in `rdi`, `rsi`, `rdx`, `r10`, `r8`, `r9` (note `r10`, not `rcx`) and executes the **`syscall`** instruction.
2. The CPU saves the user's `rip` in `rcx` and `rflags` in `r11`, switches to ring 0, and jumps to the address the kernel stored in a model-specific register called `LSTAR`. It doesn't switch stacks, so the kernel's entry code has to.
3. A small assembly stub saves registers, loads the kernel stack and calls C: effectively `syscall_dispatch(rax, rdi, rsi, rdx, ...)`.
4. The return value goes in `rax`, and `sysret` restores `rip` and `rflags` and drops back to ring 3.

Older systems used a software interrupt: `int 0x80` goes through an IDT gate you built in [lesson 5](/learn/osdev/load-tables), with the gate's privilege level set to 3 so user code may use it. It is slower, which is why `syscall` was added.

## A worked attack

Consider a *buggy* check: `if (ptr + len <= USER_TOP) { ... copy ... }`.

A malicious program calls `write(1, 0xFFFFFFFFFFFFFFF0, 0x20)`. The pointer is deep in kernel space. But `0xFFFFFFFFFFFFFFF0 + 0x20` wraps around 2^64 and equals `0x10`, which is certainly `<= USER_TOP`. The check passes, and the kernel copies 32 bytes from the top of kernel memory out to the console, or into a buffer the attacker can read. This was a real class of kernel vulnerability, and it is why range checks are written as subtractions.

Your version: `len - 1 <= USER_TOP - ptr`, guarded by `ptr <= USER_TOP`. With `ptr = 0xFFFFFFFFFFFFFFF0` the first guard already fails. With `ptr = 0x7FFFFFFFFFFFFFF0` (a valid user address) and `len = 0x20`, we get `0x1F <= 0x0F`, false, correctly rejected.

## Check, then use: and a second danger

Validating a pointer is not enough if the kernel then reads the user's memory *twice*. On a multiprocessor system another thread of the same program can change the bytes (or unmap the page) after the check and before the use. This is a **time-of-check to time-of-use** bug. The defence is the pattern you wrote: copy the data into kernel memory *once* (`copy_from_user`), and from then on work only on the copy. Looking at user memory in place, twice, is how many real exploits worked.

## Real kernels survive bad pointers

`user_range_ok` only proves a pointer is in the user half. It can't tell whether a page is mapped. A real `copy_from_user` handles that with an **exception table**: the copy loop's instructions are listed, and if a page fault happens at one of them, the fault handler doesn't kill the kernel but jumps to a recovery label that makes the copy return `-EFAULT`. That is why the tests here avoid unmapped user pointers: this kernel has no such fixup yet.

## Debugging checklist

- **A kernel pointer gets through:** the check adds `ptr + len` somewhere. Use only subtractions on the right side.
- **A huge `len` hangs the kernel:** `sys_write` must validate the whole range *before* the chunk loop, not once per chunk, and must never allocate `len` bytes.
- **`-EBADF` and `-EFAULT` come back swapped:** the order of checks matters. Check the descriptor first, then the buffer.
- **`syscall_dispatch(1000000, ...)` crashes:** you read `syscall_table[num]` before comparing `num` with the table size.
- **Registered handler never runs:** `syscall_init()` didn't fill the slot, or the numbers are wrong (0 read, 1 write, 39 getpid, 60 exit).

## Your turn

The starter provides the "rest of the OS": a console that records what was written, a pending input string, the current process id and an exit flag. You write the validation, the copy helpers, the calls and the dispatcher.
