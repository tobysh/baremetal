In Intel syntax the instructions are `push rbp`, `push rbx`, `push r12`, ... `mov [rdi], rsp`, `mov rsp, rsi`, then `pop r15`, `pop r14`, ... in the exact reverse of your pushes, and finally `ret`. Each line in the C string needs a `\n` at the end.
===
If the machine triple-faults when you first switch, the pops don't match the pushes, or `rsp` was changed *before* it was saved. Do things in this order: pushes, store old `rsp`, load new `rsp`, pops.
===
`task_stack_init`: `uint64_t *sp = stack_top; *--sp = (uint64_t)task_finished; *--sp = (uint64_t)entry; for (int i = 0; i < 6; i++) *--sp = 0; return sp;`. `*--sp` first moves the pointer down one 8-byte slot, then stores into it.
===
If tasks run but the machine faults later with a general protection fault, check alignment: the number of slots between `stack_top` and the entry address must make `rsp` equal `stack_top - 8` after the `ret`.
