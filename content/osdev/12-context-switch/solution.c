#include <stdint.h>
#include <stddef.h>

// void ctx_switch(void **save_sp, void *new_sp)
//   rdi = save_sp, rsi = new_sp (System V calling convention)
void ctx_switch(void **save_sp, void *new_sp);

__asm__(
    ".text\n"
    ".intel_syntax noprefix\n"
    ".globl ctx_switch\n"
    "ctx_switch:\n"
    "    push rbp\n"
    "    push rbx\n"
    "    push r12\n"
    "    push r13\n"
    "    push r14\n"
    "    push r15\n"
    "    mov [rdi], rsp\n"
    "    mov rsp, rsi\n"
    "    pop r15\n"
    "    pop r14\n"
    "    pop r13\n"
    "    pop r12\n"
    "    pop rbx\n"
    "    pop rbp\n"
    "    ret\n"
    ".att_syntax prefix\n");

// A task whose entry function returns lands here.
void task_finished(void) {
    for (;;) __asm__ volatile("hlt");
}

void *task_stack_init(void *stack_top, void (*entry)(void)) {
    uint64_t *sp = stack_top;
    *--sp = (uint64_t)(uintptr_t)task_finished;
    *--sp = (uint64_t)(uintptr_t)entry;
    for (int i = 0; i < 6; i++) *--sp = 0;
    return sp;
}
