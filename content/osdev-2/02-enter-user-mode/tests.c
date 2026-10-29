static uint8_t ustack[8192] __attribute__((aligned(16)));
#define UTOP ((uint64_t)(uintptr_t)(ustack + sizeof ustack))

static volatile uint64_t u_cs, u_ss, u_rsp, u_ran;

static void prog_exit(void) { u_ran = 1; __asm__ volatile("int $47"); for (;;) {} }
static void prog_info(void) {
    uint16_t cs, ss;
    uint64_t sp;
    __asm__ volatile("mov %%cs, %0" : "=r"(cs));
    __asm__ volatile("mov %%ss, %0" : "=r"(ss));
    __asm__ volatile("mov %%rsp, %0" : "=r"(sp));
    u_cs = cs; u_ss = ss; u_rsp = sp;
    __asm__ volatile("int $47");
    for (;;) {}
}
static void prog_hlt(void) { __asm__ volatile("hlt"); __asm__ volatile("int $47"); }
static void prog_in(void) { uint8_t v; __asm__ volatile("inb $0x60, %0" : "=a"(v)); __asm__ volatile("int $47"); }
static void prog_null(void) { volatile uint64_t *p = (volatile uint64_t *)8; u_ran = *p; __asm__ volatile("int $47"); }
static void prog_int3(void) { __asm__ volatile("int $3"); __asm__ volatile("int $47"); }
static void prog_kernel_write(void) {
    // Even with the U bit set everywhere, a ring 3 program may not load a kernel segment.
    __asm__ volatile("mov $0x10, %%ax\n mov %%ax, %%ss" : : : "rax");
    __asm__ volatile("int $47");
}
static void prog_clobber(void) {
    __asm__ volatile("mov $0xdead, %%rbx\n mov $0xdead, %%rbp\n mov $0xdead, %%r12\n mov $0xdead, %%r13\n mov $0xdead, %%r14\n mov $0xdead, %%r15\n int $47" : : : "rbx", "r12", "r13", "r14", "r15");
}

// Calls run_user with known callee-saved registers and reports which changed (bit 0 rbx, 1 rbp, 2 r12, 3 r13, 4 r14, 5 r15).
unsigned check_run_user(void (*entry)(void), uint64_t top);
__asm__(
    ".text\n.intel_syntax noprefix\n.globl check_run_user\ncheck_run_user:\n"
    "    push rbx\n    push rbp\n    push r12\n    push r13\n    push r14\n    push r15\n    sub rsp, 8\n"
    "    mov rbx, 0x1111\n    mov rbp, 0x2222\n    mov r12, 0x3333\n    mov r13, 0x4444\n    mov r14, 0x5555\n    mov r15, 0x6666\n"
    "    call run_user\n"
    "    xor eax, eax\n"
    "    cmp rbx, 0x1111\n    je 1f\n    or eax, 1\n1:\n"
    "    cmp rbp, 0x2222\n    je 2f\n    or eax, 2\n2:\n"
    "    cmp r12, 0x3333\n    je 3f\n    or eax, 4\n3:\n"
    "    cmp r13, 0x4444\n    je 4f\n    or eax, 8\n4:\n"
    "    cmp r14, 0x5555\n    je 5f\n    or eax, 16\n5:\n"
    "    cmp r15, 0x6666\n    je 6f\n    or eax, 32\n6:\n"
    "    add rsp, 8\n    pop r15\n    pop r14\n    pop r13\n    pop r12\n    pop rbp\n    pop rbx\n    ret\n"
    ".att_syntax prefix\n");

static void (*real_hook)(uint64_t, uint64_t, uint64_t);
static volatile uint64_t hook_sp;
static void probing_hook(uint64_t v, uint64_t e, uint64_t rip) {
    hook_sp = (uint64_t)(uintptr_t)__builtin_frame_address(0);
    real_hook(v, e, rip);
}

void kmain(void) {
    uint64_t buf[8];
    for (int i = 0; i < 8; i++) buf[i] = 0xAAAA0000 + i;
    uint64_t *f = build_iret_frame(buf + 5, 0x401000, 0x7FF0);
    CHECK("build_iret_frame returns a pointer to the rip slot, five slots below stack_top", f == buf, "returned buf + %ld", (long)(f - buf));
    CHECK("the frame holds rip, cs, rflags, rsp, ss from lowest to highest address", f == buf && buf[0] == 0x401000 && buf[1] == 0x23 && buf[3] == 0x7FF0 && buf[4] == 0x1B, "slots: %lx %lx %lx %lx %lx", buf[0], buf[1], buf[2], buf[3], buf[4]);
    CHECK("rflags has the reserved bit 1 set and interrupts and IOPL off", buf[2] == 0x2, "rflags slot is 0x%lx", buf[2]);
    CHECK("the frame does not touch memory at or above stack_top", buf[5] == 0xAAAA0005 && buf[6] == 0xAAAA0006 && buf[7] == 0xAAAA0007, "slots above the frame were overwritten");

    user_init();
    struct __attribute__((packed)) { uint16_t size; uint64_t base; } idtr;
    __asm__ volatile("sidt %0" : "=m"(idtr));
    const uint8_t *g = (const uint8_t *)(uintptr_t)idtr.base;
    CHECK("gate 47 is a DPL 3 interrupt gate (0xEE)", g[47 * 16 + 5] == 0xEE, "type byte is 0x%x", g[47 * 16 + 5]);
    CHECK("gate 3 stays DPL 0 (0x8E)", g[3 * 16 + 5] == 0x8E, "type byte is 0x%x", g[3 * 16 + 5]);
    CHECK("user_trap is installed as the exception hook", bm_exception_hook == user_trap, "bm_exception_hook is not user_trap");
    CHECK("all IRQs are masked", bm_inb(0x21) == 0xFF && bm_inb(0xA1) == 0xFF, "masks are 0x%x 0x%x", bm_inb(0x21), bm_inb(0xA1));

    uint64_t r = run_user(prog_exit, UTOP);
    CHECK("a ring 3 program runs and ends with int 47 (run_user returns 47)", r == 47 && u_ran == 1, "run_user returned %lu, ran=%lu (13 means the gate is not DPL 3)", r, (unsigned long)u_ran);
    CHECK("in_user is cleared again", in_user == 0, "in_user is still set");

    r = run_user(prog_info, UTOP);
    CHECK("inside the program cs is 0x23: the CPU really is in ring 3", u_cs == 0x23, "cs was 0x%lx", (unsigned long)u_cs);
    CHECK("inside the program ss is 0x1B", u_ss == 0x1B, "ss was 0x%lx", (unsigned long)u_ss);
    CHECK("the program starts with the stack pointer it was given", u_rsp <= UTOP && u_rsp > UTOP - 128, "rsp was 0x%lx, stack top 0x%lx", (unsigned long)u_rsp, UTOP);

    r = run_user(prog_hlt, UTOP);
    CHECK("hlt in ring 3 raises a general protection fault (vector 13)", r == 13, "run_user returned %lu", r);
    CHECK("the trap records where the faulting instruction was", last_trap_rip >= (uint64_t)(uintptr_t)prog_hlt && last_trap_rip < (uint64_t)(uintptr_t)prog_hlt + 64, "rip was 0x%lx, prog_hlt is at 0x%lx", (unsigned long)last_trap_rip, (unsigned long)(uintptr_t)prog_hlt);
    r = run_user(prog_in, UTOP);
    CHECK("port I/O from ring 3 faults (vector 13)", r == 13, "run_user returned %lu", r);
    r = run_user(prog_null, UTOP);
    CHECK("reading address 8 from ring 3 raises a page fault (vector 14)", r == 14, "run_user returned %lu", r);
    r = run_user(prog_int3, UTOP);
    CHECK("int 3 from ring 3 faults, because gate 3 is DPL 0", r == 13, "run_user returned %lu", r);
    r = run_user(prog_kernel_write, UTOP);
    CHECK("ring 3 cannot load a ring 0 data segment into ss", r == 13, "run_user returned %lu", r);

    uint64_t trap_top = (uint64_t)(uintptr_t)(trap_stack + sizeof trap_stack);
    real_hook = bm_exception_hook;
    bm_exception_hook = probing_hook;
    hook_sp = 0;
    run_user(prog_exit, UTOP);
    bm_exception_hook = real_hook;
    CHECK("the trap from ring 3 runs on the TSS rsp0 stack", hook_sp > trap_top - 1024 && hook_sp <= trap_top, "the hook ran with rsp near 0x%lx, rsp0 stack is 0x%lx", (unsigned long)hook_sp, trap_top);

    unsigned bad = check_run_user(prog_clobber, UTOP);
    CHECK("run_user preserves the kernel's callee-saved registers", bad == 0, "changed registers mask 0x%x (rbx=1 rbp=2 r12=4 r13=8 r14=16 r15=32)", bad);
    int again = 1;
    for (int i = 0; i < 50; i++) if (run_user(prog_exit, UTOP) != 47) again = 0;
    CHECK("the same machinery works 50 times in a row", again, "a run did not return 47");
}
