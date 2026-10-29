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
extern long apply_op(long op, long a, long b);
extern const char *op_name(long op);

static long ref(long op, long a, long b) {
    switch (op) {
    case 0: return (long)((unsigned long)a + (unsigned long)b);
    case 1: return (long)((unsigned long)a - (unsigned long)b);
    case 2: return (long)((unsigned long)a * (unsigned long)b);
    case 3: return a & b;
    case 4: return a | b;
    case 5: return a ^ b;
    case 6: return (long)((unsigned long)a << (b & 63));
    case 7: return a > b ? a : b;
    default: return -1;
    }
}

int main(void) {
    static const char *names[] = {"add", "sub", "mul", "and", "or", "xor", "shl", "max"};
    long vals[][2] = {{12, 5}, {-7, 3}, {0, 0}, {1000000007L, -999L}, {-1, 62}, {5, 70}, {-40, -41}, {0x7fffffffffffffffL, 1}};
    int nv = sizeof vals / sizeof vals[0];
    for (long op = 0; op < 8; op++) {
        int ok = 1; int k = 0; long got = 0;
        for (k = 0; k < nv; k++) { got = apply_op(op, vals[k][0], vals[k][1]); if (got != ref(op, vals[k][0], vals[k][1])) { ok = 0; break; } }
        char name[64]; snprintf(name, sizeof name, "op %ld (%s) on %d value pairs", op, names[op], nv);
        CHECK(name, ok, "apply_op(%ld, %ld, %ld) = %ld, expected %ld", op, vals[k % nv][0], vals[k % nv][1], got, ref(op, vals[k % nv][0], vals[k % nv][1]));
    }
    long r;
    r = apply_op(8, 1, 2);                  CHECK("op 8 is out of range", r == -1, "got %ld", r);
    r = apply_op(100, 1, 2);                CHECK("op 100 is out of range", r == -1, "got %ld", r);
    r = apply_op(-1, 1, 2);                 CHECK("op -1 is out of range (use an unsigned compare)", r == -1, "got %ld", r);
    r = apply_op((long)0x8000000000000000UL, 1, 2); CHECK("a huge negative op", r == -1, "got %ld", r);
    r = apply_op(1L << 32, 9, 9);           CHECK("op 2^32 is out of range", r == -1, "got %ld", r);

    for (long op = 0; op < 8; op++) {
        const char *s = op_name(op);
        char name[48]; snprintf(name, sizeof name, "op_name(%ld) is \"%s\"", op, names[op]);
        CHECK(name, s != NULL && strcmp(s, names[op]) == 0, "got \"%s\"", s ? s : "(null)");
    }
    const char *s = op_name(8);   CHECK("op_name(8) is \"?\"", s && strcmp(s, "?") == 0, "got \"%s\"", s ? s : "(null)");
    s = op_name(-3);              CHECK("op_name(-3) is \"?\"", s && strcmp(s, "?") == 0, "got \"%s\"", s ? s : "(null)");

    PRESERVED("apply_op preserves callee-saved registers", apply_op, 6, 3, 4);
    PRESERVED("op_name preserves callee-saved registers", op_name, 2, 0, 0);
    return 0;
}
