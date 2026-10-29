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
extern int bit_count(unsigned long x);
extern int lowest_set_index(unsigned long x);
extern unsigned long next_pow2(unsigned long n);

int main(void) {
    int r;
    r = bit_count(0);                          CHECK("bit_count(0) = 0", r == 0, "got %d", r);
    r = bit_count(1);                          CHECK("bit_count(1) = 1", r == 1, "got %d", r);
    r = bit_count(0xFF);                       CHECK("bit_count(0xFF) = 8", r == 8, "got %d", r);
    r = bit_count(0xAAAAAAAAAAAAAAAAUL);       CHECK("alternating bits", r == 32, "got %d", r);
    r = bit_count(~0UL);                       CHECK("all 64 bits", r == 64, "got %d", r);
    r = bit_count(1UL << 63);                  CHECK("only the top bit", r == 1, "got %d", r);
    int ok = 1; unsigned long x = 0x9E3779B97F4A7C15UL, i;
    for (i = 0; i < 200; i++) { x = x * 6364136223846793005UL + 1442695040888963407UL; if (bit_count(x) != __builtin_popcountl(x)) { ok = 0; break; } }
    CHECK("200 pseudo-random values", ok, "wrong for 0x%lx", x);

    r = lowest_set_index(1);                   CHECK("lowest bit of 1 is 0", r == 0, "got %d", r);
    r = lowest_set_index(0x28);                CHECK("lowest bit of 0b101000 is 3", r == 3, "got %d", r);
    r = lowest_set_index(0xF0F0);              CHECK("lowest bit of 0xF0F0 is 4", r == 4, "got %d", r);
    r = lowest_set_index(1UL << 63);           CHECK("only the top bit set", r == 63, "got %d", r);
    r = lowest_set_index(0);                   CHECK("zero gives -1", r == -1, "got %d", r);
    r = lowest_set_index(~0UL);                CHECK("all bits set gives 0", r == 0, "got %d", r);
    ok = 1; x = 12345;
    for (i = 0; i < 200; i++) { x = x * 6364136223846793005UL + 1442695040888963407UL; unsigned long v = x >> (i % 60); if (v == 0) continue; if (lowest_set_index(v) != __builtin_ctzl(v)) { ok = 0; break; } }
    CHECK("200 pseudo-random values", ok, "wrong for 0x%lx", x);

    unsigned long p;
    p = next_pow2(0);                          CHECK("next_pow2(0) = 1", p == 1, "got %lu", p);
    p = next_pow2(1);                          CHECK("next_pow2(1) = 1", p == 1, "got %lu", p);
    p = next_pow2(2);                          CHECK("next_pow2(2) = 2", p == 2, "got %lu", p);
    p = next_pow2(3);                          CHECK("next_pow2(3) = 4", p == 4, "got %lu", p);
    p = next_pow2(5);                          CHECK("next_pow2(5) = 8", p == 8, "got %lu", p);
    p = next_pow2(8);                          CHECK("exact power stays: next_pow2(8) = 8", p == 8, "got %lu", p);
    p = next_pow2(9);                          CHECK("next_pow2(9) = 16", p == 16, "got %lu", p);
    p = next_pow2(1000);                       CHECK("next_pow2(1000) = 1024", p == 1024, "got %lu", p);
    p = next_pow2((1UL << 40) + 1);            CHECK("just above 2^40", p == 1UL << 41, "got %lu", p);
    p = next_pow2(1UL << 63);                  CHECK("2^63 stays", p == 1UL << 63, "got %lu", p);
    ok = 1;
    for (i = 1; i < 5000; i++) { unsigned long e = 1; while (e < i) e <<= 1; if (next_pow2(i) != e) { ok = 0; break; } }
    CHECK("every n from 1 to 4999", ok, "wrong for n = %lu: got %lu", i, next_pow2(i));

    PRESERVED("bit_count preserves callee-saved registers", bit_count, 0xF0F0, 0, 0);
    PRESERVED("lowest_set_index preserves callee-saved registers", lowest_set_index, 40, 0, 0);
    PRESERVED("next_pow2 preserves callee-saved registers", next_pow2, 100, 0, 0);
    return 0;
}
