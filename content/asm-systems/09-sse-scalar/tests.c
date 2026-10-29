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

#include <math.h>
extern double distance(double x1, double y1, double x2, double y2);
extern double mean(const long *a, size_t n);
extern long floor_to_long(double x);

static int near(double a, double b) { return fabs(a - b) <= 1e-9 * (1.0 + fabs(b)); }

int main(void) {
    double d;
    d = distance(0, 0, 3, 4);        CHECK("3-4-5 triangle", near(d, 5.0), "got %.12g, expected 5", d);
    d = distance(1, 1, 1, 1);        CHECK("same point", near(d, 0.0), "got %.12g", d);
    d = distance(-2, -3, 4, 5);      CHECK("negative coordinates", near(d, 10.0), "got %.12g, expected 10", d);
    d = distance(0.5, 0.25, 2.5, 1.75); CHECK("fractional coordinates", near(d, 2.5), "got %.12g, expected 2.5", d);
    d = distance(1e10, 0, 0, 1e10);  CHECK("large coordinates", near(d, sqrt(2.0) * 1e10), "got %.12g", d);
    d = distance(1, 2, 3, 4);        CHECK("argument order x1 y1 x2 y2", near(d, sqrt(8.0)), "got %.12g, expected %.12g", d, sqrt(8.0));
    d = distance(0, 0, 0, -7);       CHECK("vertical", near(d, 7.0), "got %.12g, expected 7", d);

    long a[] = {1, 2, 3, 4};
    d = mean(a, 4);                  CHECK("mean of 1 2 3 4", near(d, 2.5), "got %.12g, expected 2.5", d);
    long b[] = {-10, 10, -20, 20, 5};
    d = mean(b, 5);                  CHECK("mean with negatives", near(d, 1.0), "got %.12g, expected 1", d);
    d = mean(b, 0);                  CHECK("empty array gives 0.0", d == 0.0, "got %.12g", d);
    d = mean(b + 4, 1);              CHECK("one element", near(d, 5.0), "got %.12g, expected 5", d);
    long c[] = {5000000000L, 7000000000L, -1000000000L};
    d = mean(c, 3);                  CHECK("values beyond 32 bits", near(d, 11000000000.0 / 3.0), "got %.12g", d);
    long *big = malloc(1000 * sizeof *big); double s = 0;
    for (long i = 0; i < 1000; i++) { big[i] = i * i - 300; s += (double)big[i]; }
    d = mean(big, 1000);             CHECK("1000 elements", near(d, s / 1000.0), "got %.12g, expected %.12g", d, s / 1000.0);
    free(big);

    long f;
    f = floor_to_long(2.5);          CHECK("floor(2.5) = 2", f == 2, "got %ld", f);
    f = floor_to_long(-2.5);         CHECK("floor(-2.5) = -3", f == -3, "got %ld (truncation is not floor)", f);
    f = floor_to_long(-2.0);         CHECK("floor(-2.0) = -2", f == -2, "got %ld", f);
    f = floor_to_long(0.0);          CHECK("floor(0.0) = 0", f == 0, "got %ld", f);
    f = floor_to_long(-0.25);        CHECK("floor(-0.25) = -1", f == -1, "got %ld", f);
    f = floor_to_long(0.999999);     CHECK("floor(0.999999) = 0", f == 0, "got %ld", f);
    f = floor_to_long(3e12 + 0.5);   CHECK("large positive", f == 3000000000000L, "got %ld", f);
    f = floor_to_long(-3e12 - 0.5);  CHECK("large negative", f == -3000000000001L, "got %ld", f);

    PRESERVED("preserves callee-saved registers", mean, a, 4, 0);
    return 0;
}
