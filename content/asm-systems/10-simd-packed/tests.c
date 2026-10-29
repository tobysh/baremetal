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
#include <sys/mman.h>
#include <unistd.h>
extern void add_arrays(float *dst, const float *a, const float *b, size_t n);
extern float dot_product(const float *a, const float *b, size_t n);

/* n floats placed flush against an inaccessible page, at an odd 4-byte offset from 16 alignment. */
static float *edge(size_t n) {
    long pg = sysconf(_SC_PAGESIZE);
    char *m = mmap(NULL, 3 * pg, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    mprotect(m + 2 * pg, pg, PROT_NONE);
    return (float *)(m + 2 * pg - n * sizeof(float));
}

int main(void) {
    int ok = 1; size_t n, bad = 0;
    for (n = 0; n <= 19 && ok; n++) {
        float *d = edge(n + 1), *a = edge(n), *b = edge(n);   /* d has a spare slot after n elements, directly below the guard page */
        d = d + 0;
        for (size_t i = 0; i < n; i++) { a[i] = (float)(i * 3 + 1); b[i] = (float)(100 - (long)i * 7); }
        for (size_t i = 0; i < n + 1; i++) d[i] = -1.0f;
        add_arrays(d, a, b, n);
        for (size_t i = 0; i < n; i++) if (d[i] != a[i] + b[i]) { ok = 0; bad = n; break; }
        if (d[n] != -1.0f) { ok = 0; bad = n; }
    }
    CHECK("add_arrays for every n from 0 to 19 (exact reads and writes at a page edge)", ok, "wrong result or out-of-range write for n = %zu", bad);

    float a1[8] = {1.5f, 2.5f, 3.5f, 4.5f, 5.5f, 6.5f, 7.5f, 8.5f}, b1[8] = {0.5f, 0.5f, 0.5f, 0.5f, -1, -2, -3, -4}, d1[8];
    add_arrays(d1, a1, b1, 8);
    CHECK("add_arrays with fractions and negatives", d1[0] == 2.0f && d1[3] == 5.0f && d1[4] == 4.5f && d1[7] == 4.5f, "got %g %g %g %g %g %g %g %g", d1[0], d1[1], d1[2], d1[3], d1[4], d1[5], d1[6], d1[7]);
    float inplace[6] = {1, 2, 3, 4, 5, 6};
    add_arrays(inplace, inplace, inplace, 6);
    CHECK("dst may be the same as a and b", inplace[0] == 2 && inplace[5] == 12 && inplace[4] == 10, "got %g %g %g %g %g %g", inplace[0], inplace[1], inplace[2], inplace[3], inplace[4], inplace[5]);
    float un[9] = {1, 2, 3, 4, 5, 6, 7, 8, 9}, ud[9] = {0};
    add_arrays(ud + 1, un + 1, un + 1, 5);
    CHECK("misaligned pointers work (movups) and n is respected", ud[0] == 0 && ud[1] == 4 && ud[5] == 12 && ud[6] == 0, "got %g %g %g %g %g %g %g", ud[0], ud[1], ud[2], ud[3], ud[4], ud[5], ud[6]);

    ok = 1;
    for (n = 0; n <= 19 && ok; n++) {
        float *a = edge(n), *b = edge(n); float expect = 0;
        for (size_t i = 0; i < n; i++) { a[i] = (float)((long)i % 5 - 2); b[i] = (float)(i % 7 + 1); expect += a[i] * b[i]; }
        float r = dot_product(a, b, n);
        if (r != expect) { ok = 0; bad = n; }
    }
    CHECK("dot_product for every n from 0 to 19 (exact reads at a page edge)", ok, "wrong dot product for n = %zu", bad);
    float x[] = {1, 2, 3, 4, 5}, y[] = {5, 4, 3, 2, 1};
    float r = dot_product(x, y, 5);
    CHECK("dot product of 1..5 with 5..1 is 35", r == 35.0f, "got %g", r);
    r = dot_product(x, y, 0);
    CHECK("empty dot product is 0", r == 0.0f, "got %g", r);
    r = dot_product(x, y, 3);
    CHECK("n = 3 uses only three elements", r == 22.0f, "got %g, expected 22", r);
    float h[] = {0.5f, 0.25f, 2, 4}, g[] = {2, 4, 0.5f, 0.25f};
    r = dot_product(h, g, 4);
    CHECK("fractions", r == 4.0f, "got %g, expected 4", r);

    PRESERVED4("add_arrays preserves callee-saved registers", add_arrays, d1, a1, b1, 8);
    PRESERVED("dot_product preserves callee-saved registers", dot_product, x, y, 5);
    return 0;
}
