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

extern long weighted8(long, long, long, long, long, long, long, long);
static long ref(long a, long b, long c, long d, long e, long f, long g, long h) {
    return (long)((unsigned long)a + 2UL*b + 3UL*c + 4UL*d + 5UL*e + 6UL*f + 7UL*g + 8UL*h);
}
/* weighted8 takes eight arguments, so a tiny C wrapper plays the part of bm_call. */
static long wrap(long a) { return weighted8(a, a + 1, a + 2, a + 3, a + 4, a + 5, a + 6, a + 7); }

int main(void) {
    long r;
    r = weighted8(1, 2, 3, 4, 5, 6, 7, 8);
    CHECK("1..8 gives 204", r == 204, "got %ld, expected 204", r);
    r = weighted8(1, 0, 0, 0, 0, 0, 0, 0);
    CHECK("only a set", r == 1, "got %ld, expected 1", r);
    r = weighted8(0, 1, 0, 0, 0, 0, 0, 0);
    CHECK("only b set (weight 2)", r == 2, "got %ld, expected 2", r);
    r = weighted8(0, 0, 0, 0, 0, 0, 1, 0);
    CHECK("only g set: the seventh argument is [rsp+8]", r == 7, "got %ld, expected 7", r);
    r = weighted8(0, 0, 0, 0, 0, 0, 0, 1);
    CHECK("only h set: the eighth argument is [rsp+16]", r == 8, "got %ld, expected 8", r);
    r = weighted8(-5, 10, -15, 20, -25, 30, -35, 40);
    CHECK("negative and positive mix", r == ref(-5, 10, -15, 20, -25, 30, -35, 40), "got %ld, expected %ld", r, ref(-5, 10, -15, 20, -25, 30, -35, 40));
    r = weighted8(1000000007L, -999999937L, 123456789012L, 5, -7, 11, 13, -17);
    long e = ref(1000000007L, -999999937L, 123456789012L, 5, -7, 11, 13, -17);
    CHECK("large values", r == e, "got %ld, expected %ld", r, e);
    int ok = 1; long a;
    for (a = -50; a < 50; a++) if (wrap(a) != ref(a, a+1, a+2, a+3, a+4, a+5, a+6, a+7)) { ok = 0; break; }
    CHECK("a sweep of 100 argument sets", ok, "wrong result for a = %ld (got %ld, expected %ld)", a, wrap(a), ref(a, a+1, a+2, a+3, a+4, a+5, a+6, a+7));
    /* register preservation: eight-argument call done from inline assembly */
    long bad;
    __asm__ volatile(
        "push %%rbx\n push %%r12\n push %%rbp\n"
        "movq $0x1111, %%rbx\n movq $0x2222, %%r12\n movq $0x3333, %%rbp\n"
        "sub $8, %%rsp\n"          /* keep 16-byte alignment: 3 pushes + 8 + 2 args */
        "pushq $8\n pushq $7\n"    /* 8th and 7th args (right to left) */
        "mov $1, %%edi\n mov $2, %%esi\n mov $3, %%edx\n mov $4, %%ecx\n mov $5, %%r8d\n mov $6, %%r9d\n"
        "call weighted8\n"
        "add $24, %%rsp\n"
        "xor %%eax, %%eax\n"
        "xor $0x1111, %%rbx\n or %%rbx, %%rax\n"
        "xor $0x2222, %%r12\n or %%r12, %%rax\n"
        "xor $0x3333, %%rbp\n or %%rbp, %%rax\n"
        "pop %%rbp\n pop %%r12\n pop %%rbx\n"
        : "=a"(bad)
        :
        : "rcx", "rdx", "rsi", "rdi", "r8", "r9", "r10", "r11", "memory", "cc");
    CHECK("preserves rbx, rbp and r12", bad == 0, "a callee-saved register was changed; use caller-saved registers such as r10 and r11");
    return 0;
}
