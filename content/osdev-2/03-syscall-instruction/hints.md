`wrmsr(MSR_STAR, (0x10ULL << 48) | (0x08ULL << 32));`. For EFER, read first: `wrmsr(MSR_EFER, rdmsr(MSR_EFER) | 1);`. If you write only the 1, long mode switches off and the machine dies on the next instruction.
===
`syscall_init` needs the stub's address in C: declare `void syscall_entry(void);` before the asm block, then `wrmsr(MSR_LSTAR, (uint64_t)(uintptr_t)syscall_entry);`. The asm label needs `.globl syscall_entry`.
===
Order inside the stub: first `mov [rip + user_rsp], rsp`, then `mov rsp, [rip + syscall_kernel_sp]`, then pushes. Anything pushed before the stack switch lands in user memory (or faults if the user pointed `rsp` at garbage). Save `rcx` and `r11` before you reuse `rcx` as the third C argument.
===
Pops must be the exact mirror of the pushes. After the last pop of `rcx`, restore `rsp` from `user_rsp` and execute `sysretq` (with the `q`; plain `sysret` returns to 32-bit mode).
===
`SYS_EXIT`: `in_user = 0; leave_user(a1);` and add `for (;;) {}` after it so the compiler knows the function does not fall through.
