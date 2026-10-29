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

extern unsigned long gcd_rec(unsigned long a, unsigned long b);
extern size_t count_char(const char *s, int c);

static unsigned long rg(unsigned long a, unsigned long b) { while (b) { unsigned long t = a % b; a = b; b = t; } return a; }

int main(void) {
    unsigned long r;
    r = gcd_rec(12, 18);   CHECK("gcd(12, 18) = 6", r == 6, "got %lu", r);
    r = gcd_rec(17, 5);    CHECK("coprime numbers", r == 1, "got %lu", r);
    r = gcd_rec(0, 9);     CHECK("gcd(0, 9) = 9", r == 9, "got %lu", r);
    r = gcd_rec(9, 0);     CHECK("gcd(9, 0) = 9", r == 9, "got %lu", r);
    r = gcd_rec(0, 0);     CHECK("gcd(0, 0) = 0", r == 0, "got %lu", r);
    r = gcd_rec(1071, 462);CHECK("gcd(1071, 462) = 21", r == 21, "got %lu", r);
    r = gcd_rec(3UL << 40, 9UL << 20); CHECK("large values", r == rg(3UL << 40, 9UL << 20), "got %lu, expected %lu", r, rg(3UL << 40, 9UL << 20));
    r = gcd_rec(14023930071316UL, 2282860312540UL); /* consecutive-ish Fibonacci multiples: deep recursion */
    CHECK("deep recursion", r == rg(14023930071316UL, 2282860312540UL), "got %lu, expected %lu", r, rg(14023930071316UL, 2282860312540UL));
    int ok = 1; unsigned long a = 0, b = 0;
    for (a = 0; a < 60 && ok; a++) for (b = 0; b < 60; b++) if (gcd_rec(a, b) != rg(a, b)) { ok = 0; break; }
    CHECK("all pairs below 60", ok, "wrong for gcd(%lu, %lu)", a - 1, b);

    size_t n;
    n = count_char("banana", 'a');       CHECK("three a's in banana", n == 3, "got %zu", n);
    n = count_char("banana", 'b');       CHECK("one b", n == 1, "got %zu", n);
    n = count_char("banana", 'z');       CHECK("no z", n == 0, "got %zu", n);
    n = count_char("", 'x');             CHECK("empty string", n == 0, "got %zu", n);
    n = count_char("aaaaaaaa", 'a');     CHECK("all matching", n == 8, "got %zu", n);
    n = count_char("abc", 0);            CHECK("searching for NUL gives 0", n == 0, "got %zu", n);
    n = count_char("a,b,,c,", ',');      CHECK("adjacent matches", n == 4, "got %zu", n);
    n = count_char("caf\xc3\xa9", 0xc3); CHECK("a byte above 127", n == 1, "got %zu", n);
    PRESERVED("gcd_rec preserves callee-saved registers", gcd_rec, 48, 36, 0);
    PRESERVED("count_char preserves callee-saved registers", count_char, "mississippi", 's', 0);
    return 0;
}
