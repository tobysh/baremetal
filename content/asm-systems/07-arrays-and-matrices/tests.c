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

extern void prefix_sums(long *a, size_t n);
extern long trace(const int *m, size_t n);

int main(void) {
    long a[] = {3, 1, 4, 1, 5, 9, 2, 6};
    prefix_sums(a, 8);
    long e1[] = {3, 4, 8, 9, 14, 23, 25, 31};
    CHECK("prefix sums of 8 numbers", memcmp(a, e1, sizeof a) == 0, "got %ld %ld %ld %ld %ld %ld %ld %ld", a[0], a[1], a[2], a[3], a[4], a[5], a[6], a[7]);

    long b[] = {-5, 10, -20, 100, 7, 99};
    prefix_sums(b, 4);
    long e2[] = {-5, 5, -15, 85, 7, 99};
    CHECK("negative numbers, and elements past n untouched", memcmp(b, e2, sizeof b) == 0, "got %ld %ld %ld %ld %ld %ld", b[0], b[1], b[2], b[3], b[4], b[5]);

    long c[] = {42, 77};
    prefix_sums(c, 0);
    CHECK("n = 0 changes nothing", c[0] == 42 && c[1] == 77, "got %ld %ld", c[0], c[1]);
    prefix_sums(c, 1);
    CHECK("n = 1 changes nothing", c[0] == 42 && c[1] == 77, "got %ld %ld", c[0], c[1]);

    long *big = malloc(2000 * sizeof *big);
    for (long i = 0; i < 2000; i++) big[i] = (i % 17) - 8;
    long run = 0; int ok = 1; long i;
    prefix_sums(big, 2000);
    for (i = 0; i < 2000; i++) { run += (i % 17) - 8; if (big[i] != run) { ok = 0; break; } }
    CHECK("2000 elements", ok, "wrong at index %ld: got %ld, expected %ld", i, big[i], run);
    free(big);

    int m3[] = {1, 2, 3, 4, 5, 6, 7, 8, 9};
    long t = trace(m3, 3);
    CHECK("trace of a 3x3 matrix", t == 15, "got %ld, expected 15", t);
    int m1[] = {-77};
    t = trace(m1, 1);
    CHECK("1x1 with a negative number (needs sign extension)", t == -77, "got %ld, expected -77", t);
    int m4[] = {-1, 9, 9, 9, 9, -2, 9, 9, 9, 9, -3, 9, 9, 9, 9, -4};
    t = trace(m4, 4);
    CHECK("negative diagonal", t == -10, "got %ld, expected -10", t);
    int mb[] = {2000000000, 0, 0, 2000000000};
    t = trace(mb, 2);
    CHECK("sum exceeds 32 bits", t == 4000000000L, "got %ld, expected 4000000000", t);
    t = trace(NULL, 0);
    CHECK("n = 0 returns 0 without reading memory", t == 0, "got %ld", t);

    int n;
    ok = 1;
    for (n = 1; n <= 12; n++) {
        int *m = malloc(n * n * sizeof *m); long ex = 0;
        for (int k = 0; k < n * n; k++) m[k] = (k * 37) % 101 - 50;
        for (int k = 0; k < n; k++) ex += m[k * n + k];
        if (trace(m, n) != ex) { ok = 0; free(m); break; }
        free(m);
    }
    CHECK("matrices of size 1 to 12", ok, "wrong trace for n = %d", n);

    long d[] = {1, 2, 3};
    PRESERVED("prefix_sums preserves callee-saved registers", prefix_sums, d, 3, 0);
    PRESERVED("trace preserves callee-saved registers", trace, m3, 3, 0);
    return 0;
}
