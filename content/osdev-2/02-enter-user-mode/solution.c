#include <stdint.h>
#include <stddef.h>

void outb(uint16_t port, uint8_t value);
void *memset(void *d, int c, size_t n);
extern void *bm_isr_table[48];
extern void (*bm_exception_hook)(uint64_t vector, uint64_t error_code, uint64_t rip);
void bm_exception(uint64_t vector, uint64_t error_code, uint64_t rip, uint64_t cr2);

#define USER_CS 0x23
#define USER_SS 0x1B

// ---- Provided ----

// The GDT and TSS of the last lesson, written compactly with constants:
// 0x08 kernel code, 0x10 kernel data, 0x18 user data, 0x20 user code, 0x28 TSS.
static uint64_t gdt[7];
static uint8_t tss[104] __attribute__((aligned(16)));

void gdt_setup(uint64_t kernel_stack_top) {
    memset(tss, 0, sizeof tss);
    *(uint64_t *)(tss + 4) = kernel_stack_top;        // rsp0
    *(uint16_t *)(tss + 102) = sizeof tss;            // no I/O permission bitmap
    uint64_t base = (uint64_t)(uintptr_t)tss;
    gdt[0] = 0;
    gdt[1] = 0x00AF9A000000FFFFULL;
    gdt[2] = 0x00CF92000000FFFFULL;
    gdt[3] = 0x00CFF2000000FFFFULL;
    gdt[4] = 0x00AFFA000000FFFFULL;
    gdt[5] = (sizeof tss - 1) | ((base & 0xFFFFFF) << 16) | (0x89ULL << 40) | (((base >> 24) & 0xFF) << 56);
    gdt[6] = base >> 32;
    struct __attribute__((packed)) { uint16_t size; uint64_t base; } p = { sizeof gdt - 1, (uint64_t)(uintptr_t)gdt };
    __asm__ volatile(
        "lgdt (%0)\n pushq $0x08\n leaq 1f(%%rip), %%rax\n pushq %%rax\n lretq\n 1:\n"
        "movw $0x10, %%ax\n movw %%ax, %%ds\n movw %%ax, %%es\n movw %%ax, %%ss\n"
        "movw $0x28, %%ax\n ltr %%ax\n"
        : : "r"(&p) : "rax", "memory");
}

typedef struct __attribute__((packed)) {
    uint16_t offset_low, selector;
    uint8_t ist, type_attr;
    uint16_t offset_mid;
    uint32_t offset_high, zero;
} IdtGate;

static IdtGate idt[48];

// Gates 0 to 47, all DPL 0 (type 0x8E), all pointing at the boot stub's entry code.
static void idt_init(void) {
    for (int i = 0; i < 48; i++) {
        uint64_t a = (uint64_t)(uintptr_t)bm_isr_table[i];
        idt[i] = (IdtGate){(uint16_t)a, 0x08, 0, 0x8E, (uint16_t)(a >> 16), (uint32_t)(a >> 32), 0};
    }
    struct __attribute__((packed)) { uint16_t size; uint64_t base; } p = {sizeof idt - 1, (uint64_t)(uintptr_t)idt};
    __asm__ volatile("lidt %0" : : "m"(p));
}

// Sets the U bit at every level of the page-table walk for the first 2 MiB (page 0 stays unmapped),
// so ring 3 can run the kernel's code and use its data. Far too generous for a real system.
static void make_low_memory_user(void) {
    const uint64_t addr_mask = 0x000FFFFFFFFFF000ULL;
    uint64_t cr3;
    __asm__ volatile("mov %%cr3, %0" : "=r"(cr3));
    uint64_t *pml4 = (uint64_t *)(uintptr_t)(cr3 & addr_mask);
    pml4[0] |= 4;
    uint64_t *pdpt = (uint64_t *)(uintptr_t)(pml4[0] & addr_mask);
    pdpt[0] |= 4;
    uint64_t *pd = (uint64_t *)(uintptr_t)(pdpt[0] & addr_mask);
    pd[0] |= 4;
    uint64_t *pt = (uint64_t *)(uintptr_t)(pd[0] & addr_mask);
    for (int i = 1; i < 512; i++) pt[i] |= 4;
    __asm__ volatile("mov %0, %%cr3" : : "r"(cr3) : "memory");   // flush the TLB
}

static uint8_t trap_stack[8192] __attribute__((aligned(16)));   // the kernel stack used when ring 3 traps (TSS rsp0)
static uint64_t iret_area[8] __attribute__((aligned(16)));      // room for the iretq frame

uint64_t saved_kernel_sp;                  // where enter_user left the kernel's stack pointer
volatile int in_user;                      // 1 while a ring 3 program is running
volatile uint64_t last_trap_rip, last_trap_error;

// enter_user(frame): start a ring 3 program from an iretq frame; it returns when the program traps.
// leave_user(value): abandon the trap and make enter_user return `value`.
uint64_t enter_user(uint64_t *frame);
void leave_user(uint64_t value);

// ---- Your code ----

__asm__(
    ".text\n"
    ".intel_syntax noprefix\n"
    ".globl enter_user\n"
    "enter_user:\n"
    "    push rbp\n    push rbx\n    push r12\n    push r13\n    push r14\n    push r15\n"
    "    mov [rip + saved_kernel_sp], rsp\n"
    "    mov rsp, rdi\n"
    "    iretq\n"
    ".globl leave_user\n"
    "leave_user:\n"
    "    mov rsp, [rip + saved_kernel_sp]\n"
    "    mov rax, rdi\n"
    "    pop r15\n    pop r14\n    pop r13\n    pop r12\n    pop rbx\n    pop rbp\n"
    "    ret\n"
    ".att_syntax prefix\n");

uint64_t *build_iret_frame(uint64_t *stack_top, uint64_t rip, uint64_t user_rsp) {
    uint64_t *f = stack_top;
    *--f = USER_SS;
    *--f = user_rsp;
    *--f = 0x2;              // reserved bit 1 only: interrupts off, IOPL 0
    *--f = USER_CS;
    *--f = rip;
    return f;
}

void user_trap(uint64_t vector, uint64_t error_code, uint64_t rip) {
    if (!in_user) bm_exception(vector, error_code, rip, 0);
    in_user = 0;
    last_trap_error = error_code;
    last_trap_rip = rip;
    leave_user(vector);
}

uint64_t run_user(void (*entry)(void), uint64_t user_stack_top) {
    uint64_t *frame = build_iret_frame(iret_area + 5, (uint64_t)(uintptr_t)entry, user_stack_top);
    in_user = 1;
    return enter_user(frame);
}

void user_init(void) {
    gdt_setup((uint64_t)(uintptr_t)(trap_stack + sizeof trap_stack));
    idt_init();
    idt[47].type_attr = 0xEE;       // vector 47 may be raised from ring 3
    outb(0x21, 0xFF);
    outb(0xA1, 0xFF);
    make_low_memory_user();
    bm_exception_hook = user_trap;
}
