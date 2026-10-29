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

extern void map_inplace(long *a, size_t n, long (*f)(long));
extern long fold(const long *a, size_t n, long init, long (*f)(long, long));

#define SCRIBBLE() __asm__ volatile("" ::: "rax","rcx","rdx","rsi","rdi","r8","r9","r10","r11","xmm0","xmm1","xmm2","memory")
static long calls;
__attribute__((noinline)) static long dbl(long x)  { calls++; SCRIBBLE(); return x * 2; }
__attribute__((noinline)) static long neg(long x)  { calls++; SCRIBBLE(); return -x; }
__attribute__((noinline)) static long sq1(long x)  { calls++; SCRIBBLE(); return x * x + 1; }
__attribute__((noinline)) static long add(long a, long x) { calls++; SCRIBBLE(); return a + x; }
__attribute__((noinline)) static long mx(long a, long x)  { calls++; SCRIBBLE(); return x > a ? x : a; }
__attribute__((noinline)) static long horner(long a, long x) { calls++; SCRIBBLE(); return a * 10 + x; }
/* uses an aligned SSE store: crashes if the stack is misaligned at the call */
__attribute__((noinline)) static long aligned_cb(long x) {
    volatile __attribute__((aligned(16))) long tmp[2] = { x, x };
    __asm__ volatile("movaps %%xmm0, (%0)" :: "r"(tmp) : "memory");
    SCRIBBLE(); calls++;
    return x + 1;
}

int main(void) {
    long a[6] = { 1, 2, 3, 4, 5, 6 };
    calls = 0; map_inplace(a, 6, dbl);
    CHECK("map_inplace doubles", a[0] == 2 && a[1] == 4 && a[5] == 12 && calls == 6, "a[0]=%ld a[5]=%ld, %ld calls", a[0], a[5], calls);
    long b[4] = { 5, -6, 0, 7 };
    map_inplace(b, 4, neg);
    CHECK("map_inplace negates", b[0] == -5 && b[1] == 6 && b[2] == 0 && b[3] == -7, "got %ld %ld %ld %ld", b[0], b[1], b[2], b[3]);
    long c[3] = { 9, 9, 9 };
    calls = 0; map_inplace(c, 0, sq1);
    CHECK("map_inplace with n = 0 calls nothing", calls == 0 && c[0] == 9, "%ld calls", calls);
    long d[5] = { 0, 1, 2, 3, 4 }; long g[5];
    for (int i = 0; i < 5; i++) g[i] = d[i] * d[i] + 1;
    map_inplace(d, 5, sq1);
    CHECK("map_inplace with another callback", !memcmp(d, g, sizeof d), "got %ld %ld %ld %ld %ld", d[0], d[1], d[2], d[3], d[4]);
    long e[2] = { 41, 7 };
    map_inplace(e, 2, aligned_cb);
    CHECK("stack is 16-byte aligned at the call", e[0] == 42 && e[1] == 8, "got %ld %ld", e[0], e[1]);
    long pa[3] = { 1, 2, 3 };
    PRESERVED("map_inplace preserves callee-saved registers", map_inplace, pa, 3, dbl);

    long v[6] = { 4, -2, 9, 1, 7, 3 }, r;
    r = fold(v, 6, 0, add);       CHECK("fold sum", r == 22, "got %ld", r);
    r = fold(v, 6, 100, add);     CHECK("fold sum from 100", r == 122, "got %ld", r);
    r = fold(v, 6, -1000, mx);    CHECK("fold max", r == 9, "got %ld", r);
    long dg[4] = { 1, 2, 3, 4 };
    r = fold(dg, 4, 0, horner);   CHECK("fold is applied left to right", r == 1234, "got %ld (argument order: acc first, element second)", r);
    r = fold(v, 0, 77, add);      CHECK("fold with n = 0 returns init", r == 77, "got %ld", r);
    long w[3] = { 1, 2, 3 };
    r = fold(w, 3, 0, (long (*)(long, long))aligned_cb);
    CHECK("fold keeps the stack aligned too", r == 3, "got %ld", r);
    int ok = 1;
    for (int t = 1; t < 40 && ok; t++) {
        long arr[40]; long s = 0; for (int i = 0; i < t; i++) { arr[i] = (i * 37 + t * 11) % 101 - 50; s += arr[i]; }
        if (fold(arr, t, 5, add) != s + 5) ok = 0;
    }
    CHECK("fold random lengths", ok, "a sum was wrong");
    return 0;
}
