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

extern int format_pair(char *buf, size_t n, long a, long b);
extern size_t joined_length(const char *a, const char *b);

static void fp(const char *name, long a, long b) {
    char buf[64], expect[64];
    memset(buf, 0x55, sizeof buf);
    int e = snprintf(expect, sizeof expect, "%ld,%ld", a, b);
    int r = format_pair(buf, sizeof buf, a, b);
    CHECK(name, r == e && strcmp(buf, expect) == 0, "got \"%.40s\" returning %d, expected \"%s\" returning %d", buf, r, expect, e);
}

int main(void) {
    fp("small positive numbers", 3, 4);
    fp("negative first", -4, 700);
    fp("negative second", 12, -99);
    fp("zeros", 0, 0);
    fp("large 64-bit values", 9000000000000L, -8000000000000L);
    fp("extremes", (long)0x8000000000000000UL, 0x7fffffffffffffffL);

    char small[8]; memset(small, 0x55, sizeof small);
    int r = format_pair(small, 4, 12, 3456);
    CHECK("truncates to the buffer size and still returns the full length", r == 7 && strcmp(small, "12,") == 0 && small[4] == 0x55,
          "got \"%s\" returning %d (expected \"12,\" and 7, and nothing written past 4 bytes)", small, r);
    r = format_pair(small, 0, 1, 2);
    CHECK("n = 0 writes nothing", r == 3 && small[0] == '1', "returned %d, buffer starts with 0x%02x", r, (unsigned char)small[0]);

    size_t j;
    j = joined_length("abc", "de");           CHECK("3 + 2", j == 5, "got %zu", j);
    j = joined_length("", "");                CHECK("two empty strings", j == 0, "got %zu", j);
    j = joined_length("hello world", "");     CHECK("second empty", j == 11, "got %zu", j);
    j = joined_length("", "xyz");             CHECK("first empty", j == 3, "got %zu", j);
    char *big = malloc(100001); memset(big, 'q', 100000); big[100000] = 0;
    j = joined_length(big, "12345");          CHECK("a long string", j == 100005, "got %zu", j);
    free(big);

    char buf[32];
    PRESERVED("format_pair preserves callee-saved registers", format_pair, buf, 32, 5);
    PRESERVED("joined_length preserves callee-saved registers", joined_length, "abc", "defg", 0);
    return 0;
}
