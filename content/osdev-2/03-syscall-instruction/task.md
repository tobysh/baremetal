The previous lesson's code (`run_user`, `leave_user`, `user_init`, `in_user`, all provided), `rdmsr(msr)`, `wrmsr(msr, value)`, the globals `user_rsp` and `syscall_kernel_sp`, the `syscall_stack` array and the constants `MSR_EFER`, `MSR_STAR`, `MSR_LSTAR`, `MSR_SFMASK`, `SYS_ADD`, `SYS_GETPID`, `SYS_EXIT` and `ENOSYS` are in the starter. Implement:

- `void syscall_init(void)` sets `syscall_kernel_sp` to the top of `syscall_stack`, sets bit 0 of EFER (keeping every other bit), writes STAR (kernel `cs` `0x08`, sysret base `0x10`), LSTAR (the address of `syscall_entry`) and SFMASK (`0x200`: clear IF).
- `syscall_entry` in the assembly block: the stub from the lesson. Switch to `syscall_kernel_sp` without touching the user stack, preserve `rcx`, `r11`, `rdi`, `rsi`, `rdx`, `r8`, `r9` and `r10` for the user, call `syscall_handler(rax, rdi, rsi, rdx)`, restore the user stack and `sysretq`. The result is returned in `rax`.
- `int64_t syscall_handler(uint64_t num, uint64_t a1, uint64_t a2, uint64_t a3)`: `SYS_ADD` (1) returns `a1 + a2 + a3`; `SYS_GETPID` (39) returns 42; `SYS_EXIT` (60) clears `in_user` and calls `leave_user(a1)`, so `run_user` returns the exit code; any other number returns `-ENOSYS`.

Call `user_init()` first and `syscall_init()` after it, as the tests do.
