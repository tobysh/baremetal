extern int popcount64(uint64_t x);

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

int main(void) {
    CHECK("popcount64(0) is 0", popcount64(0) == 0, "got %d", popcount64(0));
    CHECK("popcount64(1) is 1", popcount64(1) == 1, "got %d", popcount64(1));
    CHECK("popcount64(7) is 3", popcount64(7) == 3, "got %d", popcount64(7));
    CHECK("popcount64(0xFF) is 8", popcount64(0xFF) == 8, "got %d", popcount64(0xFF));
    CHECK("popcount64(0x8000000000000000) is 1 (top bit)", popcount64(0x8000000000000000ULL) == 1, "got %d", popcount64(0x8000000000000000ULL));
    CHECK("popcount64(all ones) is 64", popcount64(~0ULL) == 64, "got %d", popcount64(~0ULL));
    CHECK("popcount64(0xAAAAAAAAAAAAAAAA) is 32", popcount64(0xAAAAAAAAAAAAAAAAULL) == 32, "got %d", popcount64(0xAAAAAAAAAAAAAAAAULL));
    CHECK("popcount64(0x0123456789ABCDEF) is 32", popcount64(0x0123456789ABCDEFULL) == 32, "got %d", popcount64(0x0123456789ABCDEFULL));

    int ok = 1;
    uint64_t x = 88172645463325252ULL;
    for (int i = 0; i < 200; i++) {
        x ^= x << 13; x ^= x >> 7; x ^= x << 17;
        if (popcount64(x) != __builtin_popcountll(x)) { ok = 0; break; }
    }
    CHECK("matches the compiler's popcount on 200 pseudo-random values", ok, "mismatch for 0x%llx: got %d, expected %d", (unsigned long long)x, popcount64(x), __builtin_popcountll(x));

    long bad = -1;
    long r = bm_call((void *)popcount64, 0xF0F0, 0, 0, &bad);
    CHECK("returns the right value when called through the trampoline", (int)r == 8, "got %d", (int)r);
    CHECK("preserves rbx, rbp and r12-r15", bad == 0, "a callee-saved register was changed; use only rax, rcx, rdx, rsi, rdi, r8-r11");
    return 0;
}
