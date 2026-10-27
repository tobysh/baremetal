The `__asm__` block, `task_finished` and a stub `task_stack_init` are in the starter. Implement:

- `ctx_switch(void **save_sp, void *new_sp)` in the assembly block: push `rbp`, `rbx`, `r12`, `r13`, `r14`, `r15` (any consistent order), store `rsp` in `[rdi]`, load `rsp` from `rsi`, pop the registers in the reverse order, `ret`.
- `void *task_stack_init(void *stack_top, void (*entry)(void))` builds the initial frame shown in the lesson on a 16-byte-aligned `stack_top`: the address of `task_finished`, the address of `entry`, and six zero registers, and returns `stack_top - 64`.
