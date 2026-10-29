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

extern size_t to_decimal(char *buf, unsigned long v);

static void one(const char *name, unsigned long v) {
    char buf[40], expect[40];
    memset(buf, 0x7f, sizeof buf);
    snprintf(expect, sizeof expect, "%lu", v);
    size_t len = to_decimal(buf, v);
    size_t el = strlen(expect);
    int text = strcmp(buf, expect) == 0;
    int guard = (unsigned char)buf[el + 1] == 0x7f;
    CHECK(name, text && len == el && guard,
          "for %lu: got text \"%.25s\" length %zu, expected \"%s\" length %zu%s", v, buf, len, expect, el,
          guard ? "" : " (and you wrote past the NUL terminator)");
}

int main(void) {
    one("zero", 0);
    one("single digit", 7);
    one("ten", 10);
    one("two digits", 42);
    one("trailing zeros", 1000);
    one("1234567890", 1234567890UL);
    one("2^32", 4294967296UL);
    one("maximum 64-bit value (20 digits)", 18446744073709551615UL);
    one("2^63", 9223372036854775808UL);
    one("digits that look like a pattern", 1010101010101010101UL);
    int ok = 1; unsigned long v;
    for (v = 0; v < 3000; v += 7) {
        char buf[40], expect[40];
        snprintf(expect, sizeof expect, "%lu", v);
        to_decimal(buf, v);
        if (strcmp(buf, expect) != 0) { ok = 0; break; }
    }
    CHECK("sweep of 430 values", ok, "wrong text for %lu", v);
    char buf[40];
    PRESERVED("preserves rbx, rbp and r12-r15", to_decimal, buf, 9876543210UL, 0);
    return 0;
}
