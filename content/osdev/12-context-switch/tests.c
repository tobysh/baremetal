#define STACK 8192
static uint8_t stack_a[STACK] __attribute__((aligned(16)));
static uint8_t stack_b[STACK] __attribute__((aligned(16)));

static void *sp_main, *sp_a, *sp_b;
static volatile int ran, frame_misaligned, round_trips;
static char trace[32];
static int trace_len;

// Calls ctx_switch with known values in every callee-saved register and reports which
// ones came back different (bit 0 rbx, 1 rbp, 2 r12, 3 r13, 4 r14, 5 r15).
unsigned check_regs(void **save, void *to);
__asm__(
    ".text\n"
    ".intel_syntax noprefix\n"
    ".globl check_regs\n"
    "check_regs:\n"
    "    push rbx\n    push rbp\n    push r12\n    push r13\n    push r14\n    push r15\n"
    "    sub rsp, 8\n"
    "    mov rbx, 0x1111\n    mov rbp, 0x2222\n    mov r12, 0x3333\n    mov r13, 0x4444\n    mov r14, 0x5555\n    mov r15, 0x6666\n"
    "    call ctx_switch\n"
    "    xor eax, eax\n"
    "    cmp rbx, 0x1111\n    je 1f\n    or eax, 1\n1:\n"
    "    cmp rbp, 0x2222\n    je 2f\n    or eax, 2\n2:\n"
    "    cmp r12, 0x3333\n    je 3f\n    or eax, 4\n3:\n"
    "    cmp r13, 0x4444\n    je 4f\n    or eax, 8\n4:\n"
    "    cmp r14, 0x5555\n    je 5f\n    or eax, 16\n5:\n"
    "    cmp r15, 0x6666\n    je 6f\n    or eax, 32\n6:\n"
    "    add rsp, 8\n"
    "    pop r15\n    pop r14\n    pop r13\n    pop r12\n    pop rbp\n    pop rbx\n"
    "    ret\n"
    ".att_syntax prefix\n");

static void simple_task(void) {
    ran = 1;
    frame_misaligned = (int)((uintptr_t)__builtin_frame_address(0) & 15);
    for (;;) ctx_switch(&sp_a, sp_main);
}

static void bounce_task(void) {
    for (;;) { round_trips++; ctx_switch(&sp_a, sp_main); }
}

static void ping(void) {
    int local = 100;
    for (int i = 0; i < 3; i++) {
        trace[trace_len++] = 'A';
        local++;
        ctx_switch(&sp_a, sp_b);
    }
    if (local == 103) trace[trace_len++] = '!';
    for (;;) ctx_switch(&sp_a, sp_main);
}

static void pong(void) {
    int local = 200;
    for (;;) {
        trace[trace_len++] = 'B';
        local++;
        (void)local;
        ctx_switch(&sp_b, sp_a);
    }
}

void kmain(void) {
    uint8_t *top = stack_a + STACK;
    uint64_t *sp = task_stack_init(top, simple_task);
    CHECK("task_stack_init returns stack_top minus 64 (eight 8-byte slots)", (uint8_t *)sp == top - 64, "returned stack_top - %ld", (long)(top - (uint8_t *)sp));
    CHECK("the entry address sits above the six saved registers", sp[6] == (uint64_t)(uintptr_t)simple_task, "slot 6 holds 0x%lx", sp[6]);
    CHECK("the return address of the entry function is task_finished", sp[7] == (uint64_t)(uintptr_t)task_finished, "slot 7 holds 0x%lx", sp[7]);
    int zeros = 1;
    for (int i = 0; i < 6; i++) if (sp[i]) zeros = 0;
    CHECK("the six register slots start as zero", zeros, "a register slot is not zero");

    ran = 0;
    ctx_switch(&sp_main, sp);
    CHECK("ctx_switch starts a new task at its entry function", ran == 1, "the task never ran");
    CHECK("the task starts with a correctly aligned frame (rbp is a multiple of 16)", ran == 1 && frame_misaligned == 0, "rbp is %d bytes past a 16-byte boundary: stack_top - 8 must be what rsp is after the first ret", frame_misaligned);

    // The same task resumes where it left off, not at the start.
    round_trips = 0;
    void *b_sp = task_stack_init(stack_b + STACK, bounce_task);
    for (int i = 0; i < 10000; i++) ctx_switch(&sp_main, i == 0 ? b_sp : sp_a);
    CHECK("10,000 round trips between two stacks stay consistent", round_trips == 10000, "the task counted %d round trips", round_trips);

    trace_len = 0; memset(trace, 0, sizeof trace);
    sp_a = task_stack_init(stack_a + STACK, ping);
    sp_b = task_stack_init(stack_b + STACK, pong);
    ctx_switch(&sp_main, sp_a);
    CHECK("two tasks hand control back and forth in strict alternation", trace_len >= 6 && memcmp(trace, "ABABAB", 6) == 0, "trace was \"%s\"", trace);
    CHECK("each task's local variables survive being switched out", trace_len == 7 && trace[6] == '!', "trace was \"%s\"", trace);

    unsigned bad = check_regs(&sp_main, sp_a);
    CHECK("all six callee-saved registers survive a switch", bad == 0, "changed registers mask 0x%x (rbx=1 rbp=2 r12=4 r13=8 r14=16 r15=32)", bad);
}
