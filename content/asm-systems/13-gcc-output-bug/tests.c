/* Calls fn(a, b, c) after loading the callee-saved registers with magic
   values; *bad is set non-zero if the function changed any of them. */
long bm_call(void *fn, long a, long b, long c, long *bad);
long bm_arg4;   /* optional fourth argument (rcx) for bm_call */
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
    "    mov rcx, qword ptr [rip + bm_arg4]\n"
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

#define PRESERVED4(name, fn, a, b, c, d) do { bm_arg4 = (long)(d); PRESERVED(name, fn, a, b, c); bm_arg4 = 0; } while (0)
extern int count_above(const int *a, int n, int limit);

static int ref(const int *a, int n, int limit) { int c = 0; for (int i = 0; i < n; i++) if (a[i] > limit) c++; return c; }

/* An array with a tempting value in the slot just past the end: reading it changes the count. */
static int *make(const int *src, int n, int poison) {
    int *p = malloc((n + 1) * sizeof *p);
    for (int i = 0; i < n; i++) p[i] = src[i];
    p[n] = poison;
    return p;
}

int main(void) {
    int s1[] = {1, 5, 9, 12, 3};
    int *a = make(s1, 5, 1000);
    int r;
    r = count_above(a, 5, 4);    CHECK("positive numbers above 4", r == 3, "got %d, expected 3", r);
    r = count_above(a, 5, 100);  CHECK("none above 100", r == 0, "got %d", r);
    r = count_above(a, 5, 0);    CHECK("all above 0", r == 5, "got %d (an extra element past the end counted?)", r);
    r = count_above(a, 0, 0);    CHECK("n = 0 gives 0", r == 0, "got %d", r);
    r = count_above(a, -3, 0);   CHECK("negative n gives 0", r == 0, "got %d", r);
    free(a);

    int s2[] = {-5, -1, 0, 7, -100, 3};
    int *b = make(s2, 6, 2000000000);
    r = count_above(b, 6, -2);   CHECK("negative limit", r == 4, "got %d, expected 4", r);
    r = count_above(b, 6, -1);   CHECK("limit equal to an element is not counted", r == 3, "got %d, expected 3", r);
    r = count_above(b, 6, -101); CHECK("everything above a very negative limit", r == 6, "got %d, expected 6", r);
    r = count_above(b, 6, 7);    CHECK("nothing above the maximum", r == 0, "got %d", r);
    free(b);

    int s3[] = {2147483647, -2147483647 - 1, 0, 2147483646};
    int *c = make(s3, 4, 2147483647);
    r = count_above(c, 4, 2147483646); CHECK("INT_MAX is above INT_MAX - 1", r == 1, "got %d, expected 1", r);
    r = count_above(c, 4, -2147483647 - 1); CHECK("everything but INT_MIN is above INT_MIN", r == 3, "got %d, expected 3", r);
    free(c);

    int ok = 1; int i;
    int s4[64];
    for (i = 0; i < 64; i++) s4[i] = (i * 53) % 41 - 20;
    int *d = make(s4, 64, 40);
    for (i = -25; i <= 25; i++) for (int n = 0; n <= 64; n += 9) if (count_above(d, n, i) != ref(s4, n, i)) { ok = 0; break; }
    CHECK("sweep over limits and lengths", ok, "wrong for limit = %d", i);
    free(d);

    int e[] = {3, 4, 5};
    PRESERVED("preserves callee-saved registers", count_above, e, 3, 3);
    return 0;
}
