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
struct item { int weight; int bonus; long value; };
extern long total_score(const struct item *p, size_t n);
extern long heaviest(const struct item *p, size_t n);

static long rs(const struct item *p, size_t n) { long s = 0; for (size_t i = 0; i < n; i++) if (p[i].weight > 0) s += (long)p[i].weight * p[i].value + p[i].bonus; return s; }
static long rh(const struct item *p, size_t n) { if (!n) return -1; size_t b = 0; for (size_t i = 1; i < n; i++) if (p[i].weight > p[b].weight) b = i; return (long)b; }

int main(void) {
    struct item a[] = {{2, 1, 10}, {0, 99, 1000}, {-3, 5, 7}, {5, -2, 4}, {1, 0, -6}};
    long r;
    r = total_score(a, 5);   CHECK("mixed items", r == rs(a, 5), "got %ld, expected %ld", r, rs(a, 5));
    CHECK("the mixed items add up to 33 (21 + 18 - 6)", r == 33, "got %ld, expected 33", r);
    r = total_score(a, 0);   CHECK("n = 0 gives 0", r == 0, "got %ld", r);
    r = total_score(a + 1, 2);CHECK("only skipped items", r == 0, "got %ld, expected 0 (weight <= 0 is skipped)", r);
    r = total_score(a, 1);   CHECK("one item", r == 21, "got %ld, expected 21", r);
    struct item big[] = {{2000000000, 0, 3000000000L}, {1, -7, 1}};
    r = total_score(big, 2); CHECK("the product needs 64 bits", r == rs(big, 2), "got %ld, expected %ld", r, rs(big, 2));
    struct item neg[] = {{1, -5, -100}, {3, -1, -2}};
    r = total_score(neg, 2); CHECK("negative values and bonuses (sign extension)", r == -100 - 5 + -6 - 1, "got %ld, expected %ld", r, -112L);

    struct item *many = malloc(300 * sizeof *many);
    for (int i = 0; i < 300; i++) { many[i].weight = (i * 37) % 23 - 8; many[i].bonus = (i % 5) - 2; many[i].value = (long)i * 1000003 - 50000; }
    r = total_score(many, 300);
    CHECK("300 items", r == rs(many, 300), "got %ld, expected %ld", r, rs(many, 300));

    r = heaviest(a, 5);      CHECK("heaviest is index 3", r == 3, "got %ld", r);
    r = heaviest(a, 0);      CHECK("empty gives -1", r == -1, "got %ld", r);
    r = heaviest(a + 2, 1);  CHECK("single item is index 0", r == 0, "got %ld", r);
    struct item tie[] = {{4, 0, 0}, {9, 0, 0}, {9, 1, 1}, {2, 0, 0}};
    r = heaviest(tie, 4);    CHECK("a tie picks the first", r == 1, "got %ld", r);
    struct item allneg[] = {{-9, 0, 0}, {-2, 0, 0}, {-5, 0, 0}};
    r = heaviest(allneg, 3); CHECK("all negative weights (signed compare)", r == 1, "got %ld", r);
    struct item first[] = {{50, 0, 0}, {49, 0, 0}, {-50, 0, 0}};
    r = heaviest(first, 3);  CHECK("the first item can be the answer", r == 0, "got %ld", r);
    r = heaviest(many, 300); CHECK("300 items", r == rh(many, 300), "got %ld, expected %ld", r, rh(many, 300));
    free(many);

    PRESERVED("total_score preserves callee-saved registers", total_score, a, 5, 0);
    PRESERVED("heaviest preserves callee-saved registers", heaviest, a, 5, 0);
    return 0;
}
