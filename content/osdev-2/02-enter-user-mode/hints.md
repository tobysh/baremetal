`iretq` pops five values from the stack in a fixed order (rip, cs, rflags, ss/rsp pair), and if the target cs has a lower privilege it also switches stacks. So entering ring 3 means faking a stack frame the CPU believes came from an interrupt. Ring 3 may only use an IDT gate whose DPL allows it.
===
Four pieces: the frame builder (C), the two asm routines that swap stacks, `user_trap` / `run_user` glue, and the gate setup in `user_init`. In the asm, the pushes in `enter_user` and the pops in `leave_user` are one mirrored list. The idt entry's type byte is documented in the lesson's gate table.
===
Typical mistakes: frame slots in the wrong order (rip must end up at the lowest address), pops in the same order as the pushes instead of reversed, forgetting `.att_syntax prefix` at the end of the asm block, a gate left at DPL 0 (a ring-3 `int` then raises a general protection fault whose error code is the vector times 8 plus 2), and no valid `rsp0` in the TSS so the first trap triple-faults.
===
Stack-building idiom on a different frame: `uint32_t *p = top; *--p = c; *--p = b; *--p = a;` leaves `a` at the lowest address and returns `p`.
