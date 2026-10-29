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

extern unsigned long binom_rec(unsigned long n, unsigned long k);
extern unsigned long ackermann(unsigned long m, unsigned long n);

static unsigned long ref_binom(unsigned long n, unsigned long k) {
    if (k > n) return 0;
    unsigned long row[40] = {1};
    for (unsigned long i = 1; i <= n; i++)
        for (unsigned long j = i; j > 0; j--) row[j] += row[j - 1];
    return row[k];
}
static unsigned long ref_ack(unsigned long m, unsigned long n) {
    if (m == 0) return n + 1;
    if (n == 0) return ref_ack(m - 1, 1);
    return ref_ack(m - 1, ref_ack(m, n - 1));
}

int main(void) {
    unsigned long r;
    r = binom_rec(5, 2);  CHECK("C(5, 2) = 10", r == 10, "got %lu", r);
    r = binom_rec(0, 0);  CHECK("C(0, 0) = 1", r == 1, "got %lu", r);
    r = binom_rec(7, 0);  CHECK("C(7, 0) = 1", r == 1, "got %lu", r);
    r = binom_rec(7, 7);  CHECK("C(7, 7) = 1", r == 1, "got %lu", r);
    r = binom_rec(3, 5);  CHECK("k > n gives 0", r == 0, "got %lu", r);
    r = binom_rec(1, 4);  CHECK("C(1, 4) = 0", r == 0, "got %lu", r);

    int bad = 0;
    unsigned long first_n = 0, first_k = 0, first_got = 0;
    for (unsigned long n = 0; n <= 14; n++)
        for (unsigned long k = 0; k <= n + 1; k++) {
            unsigned long g = binom_rec(n, k);
            if (g != ref_binom(n, k) && !bad++) { first_n = n; first_k = k; first_got = g; }
        }
    CHECK("every C(n, k) for n up to 14 matches Pascal's triangle", bad == 0, "%d wrong, first was C(%lu, %lu) = %lu", bad, first_n, first_k, first_got);
    r = binom_rec(22, 11); CHECK("C(22, 11) = 705432", r == ref_binom(22, 11), "got %lu", r);
    r = binom_rec(20, 3);  CHECK("C(20, 3) = 1140", r == 1140, "got %lu", r);

    r = ackermann(0, 0);  CHECK("A(0, 0) = 1", r == 1, "got %lu", r);
    r = ackermann(0, 9);  CHECK("A(0, 9) = 10", r == 10, "got %lu", r);
    r = ackermann(1, 0);  CHECK("A(1, 0) = 2", r == 2, "got %lu", r);
    r = ackermann(2, 3);  CHECK("A(2, 3) = 9", r == 9, "got %lu", r);
    r = ackermann(3, 3);  CHECK("A(3, 3) = 61", r == 61, "got %lu", r);
    r = ackermann(4, 0);  CHECK("A(4, 0) = 13", r == 13, "got %lu", r);
    bad = 0;
    for (unsigned long m = 0; m <= 3; m++)
        for (unsigned long n = 0; n <= 8; n++)
            if (ackermann(m, n) != ref_ack(m, n)) bad++;
    CHECK("A(m, n) for m <= 3 and n <= 8 matches the C reference", bad == 0, "%d values differ", bad);

    PRESERVED("binom_rec preserves callee-saved registers", binom_rec, 12, 5, 0);
    PRESERVED("ackermann preserves callee-saved registers", ackermann, 2, 4, 0);
    return 0;
}
