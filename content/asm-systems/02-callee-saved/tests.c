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

extern size_t count_matching(const long *a, size_t n, long (*pred)(long));

volatile int bm_misaligned;
volatile long bm_calls;
long bm_is_negative(long x);
long bm_is_odd(long x);
/* Predicates written in assembly: they record misaligned calls and then scramble
   every caller-saved register, exactly what the ABI allows a callee to do. */
__asm__(
    ".intel_syntax noprefix\n"
    ".text\n"
    ".macro BM_PROLOGUE\n"
    "    lea rax, [rsp + 8]\n"
    "    test al, 15\n"
    "    jz 1f\n"
    "    mov dword ptr [rip + bm_misaligned], 1\n"
    "1:  inc qword ptr [rip + bm_calls]\n"
    "    mov rax, rdi\n"
    ".endm\n"
    ".macro BM_SCRAMBLE\n"
    "    movabs rcx, 0x7777777777777777\n    mov rdx, rcx\n    mov rsi, rcx\n    mov r8, rcx\n    mov r9, rcx\n"
    "    mov r10, rcx\n    mov r11, rcx\n    mov rdi, rcx\n"
    ".endm\n"
    "bm_is_negative:\n"
    "    BM_PROLOGUE\n"
    "    shr rax, 63\n"
    "    BM_SCRAMBLE\n"
    "    ret\n"
    "bm_is_odd:\n"
    "    BM_PROLOGUE\n"
    "    and rax, 1\n"
    "    BM_SCRAMBLE\n"
    "    ret\n"
    ".att_syntax prefix\n"
);

static size_t ref(const long *a, size_t n, long (*p)(long)) { size_t c = 0; for (size_t i = 0; i < n; i++) if (p(a[i])) c++; return c; }

int main(void) {
    long a[] = {5, -3, 8, -1, -100, 7, 0, 22, -9, 4000000000L, -4000000000L};
    size_t n = sizeof a / sizeof a[0];
    size_t r;

    r = count_matching(a, n, bm_is_negative);
    CHECK("counts negative numbers", r == 5, "got %zu, expected 5", r);
    r = count_matching(a, n, bm_is_odd);
    CHECK("counts odd numbers", r == ref(a, n, bm_is_odd), "got %zu, expected %zu", r, ref(a, n, bm_is_odd));
    r = count_matching(a, 0, bm_is_negative);
    CHECK("empty array gives 0", r == 0, "got %zu", r);
    r = count_matching(a + 2, 1, bm_is_negative);
    CHECK("single element, not matching", r == 0, "got %zu", r);
    r = count_matching(a + 1, 1, bm_is_negative);
    CHECK("single element, matching", r == 1, "got %zu", r);
    r = count_matching(a, 4, bm_is_negative);
    CHECK("respects n (first 4 elements)", r == 2, "got %zu, expected 2", r);

    long *big = malloc(5000 * sizeof *big);
    for (long i = 0; i < 5000; i++) big[i] = (i * 2654435761L) % 1000 - 500;
    r = count_matching(big, 5000, bm_is_negative);
    CHECK("5000 elements", r == ref(big, 5000, bm_is_negative), "got %zu, expected %zu", r, ref(big, 5000, bm_is_negative));
    free(big);

    bm_calls = 0;
    count_matching(a, n, bm_is_odd);
    CHECK("calls pred exactly once per element", bm_calls == (long)n, "pred was called %ld times for %zu elements", (long)bm_calls, n);
    CHECK("stack is 16-byte aligned at every call", bm_misaligned == 0, "rsp was not a multiple of 16 before a call; an even number of pushes needs an extra sub rsp, 8");
    PRESERVED("preserves rbx, rbp and r12-r15", count_matching, a, n, bm_is_negative);
    return 0;
}
