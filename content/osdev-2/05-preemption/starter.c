#include <stdint.h>
#include <stddef.h>

void outb(uint16_t port, uint8_t value);
void *memset(void *d, int c, size_t n);
extern void *bm_isr_table[48];
extern void (*bm_exception_hook)(uint64_t vector, uint64_t error_code, uint64_t rip);
void bm_exception(uint64_t vector, uint64_t error_code, uint64_t rip, uint64_t cr2);

#define MAX_TASKS  8
#define QUANTUM    5            // timer ticks per time slice
#define STACK_SIZE 8192

typedef enum { T_FREE, T_READY, T_RUNNING, T_BLOCKED, T_SLEEPING, T_DEAD } TState;

typedef struct Task {
    void *sp;                   // saved stack pointer; must stay the first field
    TState state;
    int id;
    int slice;                  // ticks left in the current time slice
    void (*entry)(void);
    uint64_t wake_tick;         // used from the sleep lesson on
    struct Task *next_waiter;   // used by wait queues
    uint8_t stack[STACK_SIZE] __attribute__((aligned(16)));
} Task;

// ---- Provided ----

static Task tasks[MAX_TASKS];   // slot 0 is the boot task, which is running kmain
static Task *current;
volatile uint64_t ticks;        // timer ticks since irq_start
uint64_t switches;              // how many times schedule() changed tasks

void task_exit(void);

// Saves the callee-saved registers on the old stack, stores rsp in *save_sp, loads new_sp and returns on the new stack.
void ctx_switch(void **save_sp, void *new_sp);
__asm__(
    ".text\n.intel_syntax noprefix\n.globl ctx_switch\nctx_switch:\n"
    "    push rbp\n    push rbx\n    push r12\n    push r13\n    push r14\n    push r15\n"
    "    mov [rdi], rsp\n    mov rsp, rsi\n"
    "    pop r15\n    pop r14\n    pop r13\n    pop r12\n    pop rbx\n    pop rbp\n    ret\n"
    ".att_syntax prefix\n");

// Every new task starts here. It arrives from inside schedule(), where interrupts are off, so turn them on.
static void task_trampoline(void) {
    __asm__ volatile("sti");
    current->entry();
    task_exit();
}

void sched_init(void) {
    memset(tasks, 0, sizeof tasks);
    tasks[0].state = T_RUNNING;
    tasks[0].slice = QUANTUM;
    current = &tasks[0];
    switches = 0;
}

// Creates a ready task in the first free or dead slot (never slot 0). Returns NULL if the table is full.
Task *task_create(void (*entry)(void)) {
    for (int i = 1; i < MAX_TASKS; i++) {
        Task *t = &tasks[i];
        if (t->state != T_FREE && t->state != T_DEAD) continue;
        t->id = i;
        t->entry = entry;
        t->slice = QUANTUM;
        t->wake_tick = 0;
        t->next_waiter = NULL;
        uint64_t *sp = (uint64_t *)(t->stack + STACK_SIZE);
        *--sp = 0;                                      // fake return address of the trampoline
        *--sp = (uint64_t)(uintptr_t)task_trampoline;   // where ctx_switch's ret goes
        for (int r = 0; r < 6; r++) *--sp = 0;          // rbp, rbx, r12 to r15
        t->sp = sp;
        t->state = T_READY;
        return t;
    }
    return NULL;
}

// Interrupts: the IDT, the PIC remapped to 32/40 with only the timer unmasked, the PIT at `hz`, and sti.
typedef struct __attribute__((packed)) {
    uint16_t offset_low, selector;
    uint8_t ist, type_attr;
    uint16_t offset_mid;
    uint32_t offset_high, zero;
} IdtGate;
static IdtGate idt[48];

static void pic_eoi(void) { outb(0x20, 0x20); }

void timer_tick(void);

static void irq_dispatch(uint64_t vector, uint64_t error_code, uint64_t rip) {
    if (vector == 32) timer_tick();
    else if (vector < 32) bm_exception(vector, error_code, rip, 0);
}

void irq_start(uint32_t hz) {
    for (int i = 0; i < 48; i++) {
        uint64_t a = (uint64_t)(uintptr_t)bm_isr_table[i];
        idt[i] = (IdtGate){(uint16_t)a, 0x08, 0, 0x8E, (uint16_t)(a >> 16), (uint32_t)(a >> 32), 0};
    }
    struct __attribute__((packed)) { uint16_t size; uint64_t base; } p = {sizeof idt - 1, (uint64_t)(uintptr_t)idt};
    __asm__ volatile("lidt %0" : : "m"(p));
    outb(0x20, 0x11); outb(0xA0, 0x11);
    outb(0x21, 32); outb(0xA1, 40);
    outb(0x21, 4); outb(0xA1, 2);
    outb(0x21, 1); outb(0xA1, 1);
    outb(0x21, 0xFE); outb(0xA1, 0xFF);                 // only IRQ 0
    uint32_t div = 1193182 / hz;
    outb(0x43, 0x36); outb(0x40, (uint8_t)div); outb(0x40, (uint8_t)(div >> 8));
    bm_exception_hook = irq_dispatch;
    __asm__ volatile("sti");
}

// ---- Your code ----

unsigned long irq_save(void) {
    return 0;
}

void irq_restore(unsigned long flags) {
}

Task *pick_next(void) {
    return &tasks[0];
}

void schedule(void) {
}

void yield(void) {
}

void timer_tick(void) {
}

void task_exit(void) {
    for (;;) __asm__ volatile("hlt");
}
