`syscall` is a fast ring change that does almost nothing for you: it puts the return address in `rcx` and rflags in `r11`, loads cs/ss from STAR, jumps to LSTAR, and leaves `rsp` as the user's. The kernel stub must switch stacks and save state itself, and `sysretq` reverses it.
===
Work in three places: `syscall_init` (EFER, STAR, LSTAR, SFMASK via `wrmsr`), the asm stub `syscall_entry`, and the C `syscall_handler` switch. The lesson's tables give the bit layout of STAR. Declare `syscall_entry` in C and make the label global so C can take its address.
===
Typical mistakes: overwriting EFER instead of or-ing in the SCE bit (long mode then switches off and the machine dies), pushing before the stack switch so data lands in user memory, clobbering `rcx` or `r11` when setting up the C call arguments, pops that are not an exact mirror of the pushes, plain `sysret` instead of `sysretq`, and a handler case that falls through after a call that never returns.
===
Read-modify-write on a different register: `wrmsr(SOME_MSR, rdmsr(SOME_MSR) | (1ULL << 3));` changes one bit and keeps the rest.
