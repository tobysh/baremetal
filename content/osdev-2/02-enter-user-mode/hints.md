`build_iret_frame`: `uint64_t *f = stack_top; *--f = USER_SS; *--f = user_rsp; *--f = 0x2; *--f = USER_CS; *--f = rip; return f;`. Each `*--f` moves the pointer down one slot, then stores, so the first value stored ends up highest.
===
`enter_user` in Intel syntax: six `push` lines, then `mov [rip + saved_kernel_sp], rsp`, `mov rsp, rdi`, `iretq`. `leave_user` is the mirror image: `mov rsp, [rip + saved_kernel_sp]`, `mov rax, rdi`, six `pop` lines in the reverse order of the pushes, `ret`. Declare both with `.globl` and end the block with `.att_syntax prefix`.
===
If the machine triple-faults at once, either the iretq frame is in the wrong order (rip must be at the lowest address) or `user_init` did not give the CPU a valid `rsp0`. If you get a general protection fault with error code 0x17A when the program does `int $47`, the gate still has DPL 0: 0x17A is `47 * 8 + 2`, the IDT index the CPU refused.
===
The type byte is `idt[47].type_attr = 0xEE;`. `0x8E` is present, DPL 0, interrupt gate. `0xEE` is the same with DPL 3 (bits 5-6 set).
