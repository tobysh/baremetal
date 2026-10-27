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
    "    ret\n"
    ".att_syntax prefix\n");

// A task whose entry function returns lands here.
void task_finished(void) {
    for (;;) __asm__ volatile("hlt");
}

void *task_stack_init(void *stack_top, void (*entry)(void)) {
    return stack_top;
}
