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
extern long vm_run(const unsigned char *code);

enum { HALT, PUSH, ADD, SUB, MUL, DUP, SWAP, POP, JNZ };

/* The reference interpreter, in C. */
static long ref(const unsigned char *code) {
    long st[256]; int sp = 0; size_t pc = 0;
    for (;;) {
        switch (code[pc++]) {
        case HALT: return st[sp - 1];
        case PUSH: st[sp++] = (signed char)code[pc++]; break;
        case ADD: sp--; st[sp - 1] += st[sp]; break;
        case SUB: sp--; st[sp - 1] -= st[sp]; break;
        case MUL: sp--; st[sp - 1] *= st[sp]; break;
        case DUP: st[sp] = st[sp - 1]; sp++; break;
        case SWAP: { long t = st[sp - 1]; st[sp - 1] = st[sp - 2]; st[sp - 2] = t; break; }
        case POP: sp--; break;
        case JNZ: { size_t t = code[pc++]; if (st[--sp] != 0) pc = t; break; }
        default: return -1;
        }
    }
}

static void prog(const char *name, const unsigned char *code, long expect) {
    long r = vm_run(code);
    CHECK(name, r == expect && r == ref(code), "got %ld, expected %ld", r, expect);
}

int main(void) {
    unsigned char p1[] = {PUSH, 3, PUSH, 4, ADD, HALT};
    prog("3 + 4 = 7", p1, 7);
    unsigned char p2[] = {PUSH, 10, PUSH, 4, SUB, HALT};
    prog("10 - 4: SUB takes second-from-top minus top", p2, 6);
    unsigned char p3[] = {PUSH, 4, PUSH, 10, SUB, HALT};
    prog("4 - 10 = -6", p3, -6);
    unsigned char p4[] = {PUSH, 7, DUP, MUL, HALT};
    prog("DUP and MUL: 7 squared", p4, 49);
    unsigned char p5[] = {PUSH, 3, PUSH, 10, SWAP, SUB, HALT};
    prog("SWAP changes the order: 10 - 3", p5, 7);
    unsigned char p6[] = {PUSH, 0x80, PUSH, 4, MUL, HALT};
    prog("PUSH sign-extends: -128 * 4", p6, -512);
    unsigned char p7[] = {PUSH, 5, PUSH, 9, POP, HALT};
    prog("POP discards the top", p7, 5);
    unsigned char p8[] = {PUSH, 0, JNZ, 9, PUSH, 42, HALT};
    prog("JNZ not taken on zero", p8, 42);
    unsigned char p9[] = {PUSH, 1, JNZ, 6, PUSH, 100, HALT, PUSH, 7, HALT};
    /* offsets: 0 PUSH 1, 2 JNZ 6, 4 PUSH 100, 6 HALT?  Jump to 7 instead: */
    p9[3] = 7;
    prog("JNZ taken jumps over code", p9, 7);
    unsigned char p10[] = {PUSH, -3, PUSH, 4, MUL, DUP, ADD, HALT};
    prog("(-3 * 4) doubled", p10, -24);
    unsigned char p11[] = {PUSH, 5, 99};
    prog("unknown opcode returns -1", p11, -1);
    unsigned char p12[] = {PUSH, 2, PUSH, 3, PUSH, 4, MUL, ADD, PUSH, 5, SWAP, SUB, HALT};
    prog("2 + 3*4 then 5 swapped under: 5 - 14", p12, -9);

    /* 2^n by repeated doubling, with a loop */
    int ok = 1; long got = 0, want = 0; int n;
    for (n = 1; n <= 40; n++) {
        unsigned char loop[] = {PUSH, 1, PUSH, (unsigned char)(n > 127 ? 127 : n), SWAP, DUP, ADD, SWAP, PUSH, 1, SUB, DUP, JNZ, 4, POP, HALT};
        got = vm_run(loop); want = 1L << n;
        if (n > 62) break;
        if (got != want) { ok = 0; break; }
    }
    CHECK("loop program computes 2^n for n = 1..40", ok, "n = %d: got %ld, expected %ld", n, got, want);

    /* countdown: sum 0 + 1 + ... via repeated work is hard on two slots, so check a pure countdown */
    int ok2 = 1;
    for (n = 1; n <= 100; n++) {
        unsigned char cd[] = {PUSH, (unsigned char)n, PUSH, 1, SUB, DUP, JNZ, 2, HALT};
        got = vm_run(cd);
        if (got != 0) { ok2 = 0; break; }
    }
    CHECK("countdown loop ends at 0 for every start from 1 to 100", ok2, "start %d: got %ld", n, got);

    /* many pushes: the VM stack lives on the real stack */
    unsigned char deep[200]; int k = 0;
    for (int i = 0; i < 60; i++) { deep[k++] = PUSH; deep[k++] = (unsigned char)(i + 1); }
    for (int i = 0; i < 59; i++) deep[k++] = ADD;
    deep[k++] = HALT;
    got = vm_run(deep);
    CHECK("60 values on the stack, then 59 additions", got == 1830, "got %ld, expected 1830", got);

    /* the real stack pointer must be restored on every exit path */
    long before, after;
    __asm__ volatile("mov %%rsp, %0" : "=r"(before));
    vm_run(p1); vm_run(p11); vm_run(p7); vm_run(deep);
    __asm__ volatile("mov %%rsp, %0" : "=r"(after));
    CHECK("rsp is unchanged after HALT and after a bad opcode", before == after, "rsp moved by %ld bytes", after - before);

    PRESERVED("preserves callee-saved registers", vm_run, p4, 0, 0);
    return 0;
}
