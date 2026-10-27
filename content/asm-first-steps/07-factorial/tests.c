extern unsigned long factorial(unsigned long n);

/* Calls fn(a, b, c) after loading the callee-saved registers with magic
   values; *bad is set non-zero if the function changed any of them. */
long bm_call(void *fn, long a, long b, long c, long *bad);
__asm__(
    ".intel_syntax noprefix\n"
    ".text\n"
    "bm_call:\n"
    "    push rbx\n    push rbp\n    push r12\n    push r13\n    push r14\n    push r15\n"
    "    push r8\n"
    "    mov rax, rdi\n    mov rdi, rsi\n    mov rsi, rdx\n    mov rdx, rcx\n"
    "    movabs rbx, 0x1111111111111111\n    movabs rbp, 0x2222222222222222\n"
    "    movabs r12, 0x3333333333333333\n    movabs r13, 0x4444444444444444\n"
    "    movabs r14, 0x5555555555555555\n    movabs r15, 0x6666666666666666\n"
    "    call rax\n"
    "    pop r8\n"
    "    xor r10d, r10d\n"
    "    movabs r9, 0x1111111111111111\n    xor r9, rbx\n    or r10, r9\n"
    "    movabs r9, 0x2222222222222222\n    xor r9, rbp\n    or r10, r9\n"
    "    movabs r9, 0x3333333333333333\n    xor r9, r12\n    or r10, r9\n"
    "    movabs r9, 0x4444444444444444\n    xor r9, r13\n    or r10, r9\n"
    "    movabs r9, 0x5555555555555555\n    xor r9, r14\n    or r10, r9\n"
    "    movabs r9, 0x6666666666666666\n    xor r9, r15\n    or r10, r9\n"
    "    mov [r8], r10\n"
    "    pop r15\n    pop r14\n    pop r13\n    pop r12\n    pop rbp\n    pop rbx\n"
    "    ret\n"
    ".att_syntax prefix\n"
);

static unsigned long fact_ref(unsigned long n) { unsigned long r = 1; while (n > 1) r *= n--; return r; }

int main(void) {
    CHECK("factorial(0) is 1", factorial(0) == 1, "got %lu", factorial(0));
    CHECK("factorial(1) is 1", factorial(1) == 1, "got %lu", factorial(1));
    CHECK("factorial(2) is 2", factorial(2) == 2, "got %lu", factorial(2));
    CHECK("factorial(5) is 120", factorial(5) == 120, "got %lu", factorial(5));
    CHECK("factorial(10) is 3628800", factorial(10) == 3628800UL, "got %lu", factorial(10));
    CHECK("factorial(20) is 2432902008176640000", factorial(20) == 2432902008176640000UL, "got %lu", factorial(20));

    int ok = 1; unsigned long n;
    for (n = 0; n <= 20; n++) if (factorial(n) != fact_ref(n)) { ok = 0; break; }
    CHECK("matches an iterative C version for 0..20", ok, "differs at n = %lu: got %lu, expected %lu", n, factorial(n), fact_ref(n));

    long bad = -1;
    long r = bm_call((void *)factorial, 12, 0, 0, &bad);
    CHECK("works when called with callee-saved registers loaded", (unsigned long)r == 479001600UL, "got %lu", (unsigned long)r);
    CHECK("preserves rbx, rbp and r12-r15", bad == 0, "a callee-saved register was changed: push it on entry and pop it before ret");

    bad = -1;
    r = bm_call((void *)factorial, 1, 0, 0, &bad);
    CHECK("base case also preserves callee-saved registers", r == 1 && bad == 0, "result %ld, registers %s", r, bad ? "changed" : "intact");
    return 0;
}
