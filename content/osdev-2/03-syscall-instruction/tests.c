static uint8_t ustack[8192] __attribute__((aligned(16)));
#define UTOP ((uint64_t)(uintptr_t)(ustack + sizeof ustack))

static volatile int64_t u_result, u_result2, u_cs, u_ss, u_flag;
static volatile uint64_t u_mask, u_rcx_ok;

// A ring 3 program makes a system call with the number in rax and arguments in rdi, rsi, rdx.
#define SYSCALL3(n, a, b, c, out) __asm__ volatile("syscall" : "=a"(out) : "a"((uint64_t)(n)), "D"((uint64_t)(a)), "S"((uint64_t)(b)), "d"((uint64_t)(c)) : "rcx", "r11", "memory")
#define EXIT(code) do { int64_t _r; SYSCALL3(60, code, 0, 0, _r); for (;;) {} } while (0)

static void prog_add(void) { int64_t r; SYSCALL3(1, 10, 20, 30, r); u_result = r; SYSCALL3(39, 0, 0, 0, r); u_result2 = r; EXIT(5); }
static void prog_unknown(void) { int64_t r; SYSCALL3(999, 1, 2, 3, r); u_result = r; EXIT(0); }
static void prog_ring(void) {
    int64_t r; uint16_t cs, ss;
    SYSCALL3(1, 1, 1, 1, r);
    __asm__ volatile("mov %%cs, %0" : "=r"(cs));
    __asm__ volatile("mov %%ss, %0" : "=r"(ss));
    u_cs = cs; u_ss = ss; EXIT(0);
}
static void prog_rcx(void) {
    uint64_t after, rcx, r11, fl;
    __asm__ volatile("lea 1f(%%rip), %0\n mov $1, %%eax\n xor %%edi, %%edi\n xor %%esi, %%esi\n xor %%edx, %%edx\n syscall\n 1: mov %%rcx, %1\n mov %%r11, %2"
                     : "=&r"(after), "=&r"(rcx), "=&r"(r11) : : "rax", "rdi", "rsi", "rdx", "rcx", "r11", "memory");
    fl = r11;
    u_rcx_ok = rcx == after; u_flag = (int64_t)fl;
    EXIT(0);
}
static void prog_regs(void) {
    uint64_t bad;
    __asm__ volatile(
        "mov $0x1111, %%rbx\n mov $0x2222, %%r12\n mov $0x3333, %%r13\n mov $0x4444, %%r14\n mov $0x5555, %%r15\n"
        "mov $0x6666, %%r8\n mov $0x7777, %%r9\n mov $0x8888, %%r10\n"
        "mov $1, %%eax\n mov $0x9999, %%edi\n mov $0xAAAA, %%esi\n mov $0xBBBB, %%edx\n"
        "syscall\n"
        "xor %0, %0\n"
        "cmp $0x1111, %%rbx\n je 1f\n or $1, %0\n 1:\n"
        "cmp $0x2222, %%r12\n je 2f\n or $2, %0\n 2:\n"
        "cmp $0x3333, %%r13\n je 3f\n or $4, %0\n 3:\n"
        "cmp $0x4444, %%r14\n je 4f\n or $8, %0\n 4:\n"
        "cmp $0x5555, %%r15\n je 5f\n or $16, %0\n 5:\n"
        "cmp $0x6666, %%r8\n je 6f\n or $32, %0\n 6:\n"
        "cmp $0x7777, %%r9\n je 7f\n or $64, %0\n 7:\n"
        "cmp $0x8888, %%r10\n je 8f\n or $128, %0\n 8:\n"
        "cmp $0x9999, %%edi\n je 9f\n or $256, %0\n 9:\n"
        "cmp $0xAAAA, %%esi\n je 10f\n or $512, %0\n 10:\n"
        "cmp $0xBBBB, %%edx\n je 11f\n or $1024, %0\n 11:\n"
        : "=&a"(bad) : : "rbx", "r12", "r13", "r14", "r15", "r8", "r9", "r10", "rdi", "rsi", "rdx", "rcx", "r11", "cc", "memory");
    u_mask = bad;
    EXIT(0);
}
static void prog_badstack(void) {
    // The user's stack pointer is garbage. A kernel that pushes onto it faults.
    uint64_t save, r;
    __asm__ volatile("mov %%rsp, %0\n mov $0x10, %%rsp\n mov $1, %%eax\n mov $7, %%edi\n mov $8, %%esi\n mov $9, %%edx\n syscall\n mov %%rax, %1\n mov %0, %%rsp"
                     : "=&r"(save), "=&r"(r) : : "rax", "rdi", "rsi", "rdx", "rcx", "r11", "memory");
    u_result = (int64_t)r;
    EXIT(0);
}
static void prog_hlt(void) { __asm__ volatile("hlt"); EXIT(0); }

void kmain(void) {
    user_init();
    syscall_init();

    uint64_t efer = rdmsr(MSR_EFER), star = rdmsr(MSR_STAR);
    CHECK("EFER.SCE is set", (efer & 1) == 1, "EFER is 0x%lx", efer);
    CHECK("EFER keeps long mode enabled (LME and LMA)", (efer & 0x500) == 0x500, "EFER is 0x%lx", efer);
    CHECK("STAR bits 47:32 hold the kernel code selector 0x08", ((star >> 32) & 0xFFFF) == 0x08, "STAR is 0x%lx", star);
    CHECK("STAR bits 63:48 hold the sysret base 0x10 (user SS 0x18, user CS 0x20)", ((star >> 48) & 0xFFFF) == 0x10, "STAR is 0x%lx", star);
    CHECK("LSTAR points at syscall_entry", rdmsr(MSR_LSTAR) == (uint64_t)(uintptr_t)syscall_entry, "LSTAR is 0x%lx", rdmsr(MSR_LSTAR));
    CHECK("SFMASK clears the interrupt flag", (rdmsr(MSR_SFMASK) & 0x200) != 0, "SFMASK is 0x%lx", rdmsr(MSR_SFMASK));
    CHECK("syscall_kernel_sp is set to the top of a kernel stack", syscall_kernel_sp != 0 && (syscall_kernel_sp & 15) == 0, "syscall_kernel_sp is 0x%lx", syscall_kernel_sp);

    int64_t h = syscall_handler(1, 2, 3, 4);
    CHECK("syscall_handler adds three numbers for SYS_ADD", h == 9, "returned %ld", (long)h);
    CHECK("syscall_handler returns 42 for SYS_GETPID", syscall_handler(39, 0, 0, 0) == 42, "returned %ld", (long)syscall_handler(39, 0, 0, 0));
    CHECK("syscall_handler returns -ENOSYS for an unknown number", syscall_handler(1000, 0, 0, 0) == -38, "returned %ld", (long)syscall_handler(1000, 0, 0, 0));

    u_result = u_result2 = -1;
    uint64_t r = run_user(prog_add, UTOP);
    CHECK("a ring 3 syscall passes three arguments and gets the sum back in rax", u_result == 60, "SYS_ADD(10, 20, 30) gave %ld (run_user returned %lu; 6 means #UD: syscall is not enabled)", (long)u_result, r);
    CHECK("a second syscall in the same program works (getpid is 42)", u_result2 == 42, "SYS_GETPID gave %ld", (long)u_result2);
    CHECK("SYS_EXIT ends the program and run_user returns the exit code", r == 5, "run_user returned %lu", r);

    r = run_user(prog_unknown, UTOP);
    CHECK("an unknown syscall number returns -ENOSYS to ring 3", u_result == -38, "got %ld", (long)u_result);

    r = run_user(prog_ring, UTOP);
    CHECK("sysretq returns to ring 3 (cs is 0x23)", u_cs == 0x23, "cs is 0x%lx", (unsigned long)u_cs);
    CHECK("sysretq returns with the user stack segment (ss is 0x1B)", u_ss == 0x1B, "ss is 0x%lx", (unsigned long)u_ss);

    r = run_user(prog_rcx, UTOP);
    CHECK("the syscall instruction saves the return address in rcx and the stub keeps it", u_rcx_ok == 1, "rcx after the syscall was not the address after the instruction");
    CHECK("rflags comes back as the user had it (r11): reserved bit 1 set, interrupts still off", (u_flag & 0x202) == 0x2, "rflags was 0x%lx", (unsigned long)u_flag);

    r = run_user(prog_regs, UTOP);
    CHECK("a syscall preserves rbx and r12 to r15", (u_mask & 31) == 0, "changed mask 0x%lx (rbx=1 r12=2 r13=4 r14=8 r15=16)", (unsigned long)u_mask);
    CHECK("a syscall preserves r8, r9 and r10", (u_mask & (32 | 64 | 128)) == 0, "changed mask 0x%lx (r8=32 r9=64 r10=128)", (unsigned long)u_mask);
    CHECK("a syscall preserves rdi, rsi and rdx", (u_mask & (256 | 512 | 1024)) == 0, "changed mask 0x%lx (rdi=256 rsi=512 rdx=1024)", (unsigned long)u_mask);

    u_result = -1;
    r = run_user(prog_badstack, UTOP);
    CHECK("the kernel never uses the user's stack (rsp = 0x10 during the call)", u_result == 24 && r == 0, "result %ld, run_user returned %lu (14 means a page fault: the stub pushed onto the user stack)", (long)u_result, r);

    r = run_user(prog_hlt, UTOP);
    CHECK("exceptions from ring 3 still end the program (hlt gives vector 13)", r == 13, "run_user returned %lu", r);

    int again = 1;
    for (int i = 0; i < 100; i++) { u_result = 0; if (run_user(prog_add, UTOP) != 5 || u_result != 60) again = 0; }
    CHECK("100 programs in a row, each with its own syscalls", again, "a run went wrong");
}
