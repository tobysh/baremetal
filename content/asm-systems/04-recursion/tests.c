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
#define PRESERVED(name, fn, a, b, c) do { long bad_ = -1; bm_call((void *)(fn), (long)(a), (long)(b), (long)(c), &bad_); \
    CHECK(name, bad_ == 0, "a callee-saved register (rbx, rbp, r12-r15) was changed; save it before use and restore it before ret"); } while (0)

extern unsigned long fact_rec(unsigned long n);
extern unsigned long fib_rec(unsigned long n);

static unsigned long rf(unsigned long n) { unsigned long r = 1; for (unsigned long i = 2; i <= n; i++) r *= i; return r; }
static unsigned long rb(unsigned long n) { unsigned long a = 0, b = 1; for (unsigned long i = 0; i < n; i++) { unsigned long t = a + b; a = b; b = t; } return a; }

int main(void) {
    unsigned long r;
    r = fact_rec(0);  CHECK("0! = 1", r == 1, "got %lu", r);
    r = fact_rec(1);  CHECK("1! = 1", r == 1, "got %lu", r);
    r = fact_rec(5);  CHECK("5! = 120", r == 120, "got %lu", r);
    r = fact_rec(10); CHECK("10! = 3628800", r == 3628800UL, "got %lu", r);
    r = fact_rec(20); CHECK("20! = 2432902008176640000", r == 2432902008176640000UL, "got %lu", r);
    r = fact_rec(25); CHECK("25! wraps modulo 2^64", r == rf(25), "got %lu, expected %lu", r, rf(25));
    int ok = 1; unsigned long n;
    for (n = 0; n <= 30; n++) if (fact_rec(n) != rf(n)) { ok = 0; break; }
    CHECK("factorial for every n up to 30", ok, "wrong for n = %lu: got %lu, expected %lu", n, fact_rec(n), rf(n));

    r = fib_rec(0);  CHECK("fib(0) = 0", r == 0, "got %lu", r);
    r = fib_rec(1);  CHECK("fib(1) = 1", r == 1, "got %lu", r);
    r = fib_rec(2);  CHECK("fib(2) = 1", r == 1, "got %lu", r);
    r = fib_rec(10); CHECK("fib(10) = 55", r == 55, "got %lu", r);
    r = fib_rec(25); CHECK("fib(25) = 75025", r == 75025, "got %lu", r);
    ok = 1;
    for (n = 0; n <= 22; n++) if (fib_rec(n) != rb(n)) { ok = 0; break; }
    CHECK("Fibonacci for every n up to 22", ok, "wrong for n = %lu: got %lu, expected %lu", n, fib_rec(n), rb(n));

    PRESERVED("fact_rec preserves rbx, rbp and r12-r15", fact_rec, 12, 0, 0);
    PRESERVED("fib_rec preserves rbx, rbp and r12-r15", fib_rec, 15, 0, 0);
    return 0;
}
