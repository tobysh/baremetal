extern long binary_search(const long *a, size_t n, long key);

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
    long a[] = {-40, -3, 0, 2, 9, 17, 100, 5000000000L};
    size_t n = sizeof a / sizeof a[0];
    long poison[1] = {123};

    CHECK("finds the first element", binary_search(a, n, -40) == 0, "got %ld", binary_search(a, n, -40));
    CHECK("finds the last element", binary_search(a, n, 5000000000L) == 7, "got %ld", binary_search(a, n, 5000000000L));
    CHECK("finds a middle element", binary_search(a, n, 9) == 4, "got %ld", binary_search(a, n, 9));
    CHECK("finds zero", binary_search(a, n, 0) == 2, "got %ld", binary_search(a, n, 0));
    CHECK("finds a negative number", binary_search(a, n, -3) == 1, "got %ld", binary_search(a, n, -3));
    CHECK("missing value below the range", binary_search(a, n, -100) == -1, "got %ld", binary_search(a, n, -100));
    CHECK("missing value above the range", binary_search(a, n, 6000000000L) == -1, "got %ld", binary_search(a, n, 6000000000L));
    CHECK("missing value in the middle", binary_search(a, n, 10) == -1, "got %ld", binary_search(a, n, 10));
    CHECK("empty array returns -1 without reading it", binary_search(poison, 0, 123) == -1, "got %ld", binary_search(poison, 0, 123));
    CHECK("one element: present", binary_search(poison, 1, 123) == 0, "got %ld", binary_search(poison, 1, 123));
    CHECK("one element: absent", binary_search(poison, 1, 5) == -1, "got %ld", binary_search(poison, 1, 5));

    long *big = malloc(1000 * sizeof *big);
    for (long i = 0; i < 1000; i++) big[i] = i * 7 - 2000;
    int ok = 1; long i;
    for (i = 0; i < 1000; i++) if (binary_search(big, 1000, big[i]) != i) { ok = 0; break; }
    CHECK("finds every element of a 1000-element array", ok, "wrong index for element %ld: got %ld", i, binary_search(big, 1000, big[i]));
    ok = 1;
    for (i = 0; i < 1000; i++) if (binary_search(big, 1000, big[i] + 1) != -1) { ok = 0; break; }
    CHECK("reports -1 for every gap between elements", ok, "found a key that is not there (searching for %ld)", big[i] + 1);

    long dup[] = {1, 3, 3, 3, 3, 8};
    long d = binary_search(dup, 6, 3);
    CHECK("with duplicates, returns an index that holds the key", d >= 1 && d <= 4, "got %ld", d);

    long bad = -1;
    long r = bm_call((void *)binary_search, (long)(intptr_t)a, (long)n, 17, &bad);
    CHECK("works with callee-saved registers loaded", r == 5, "got %ld", r);
    CHECK("preserves rbx, rbp and r12-r15", bad == 0, "a callee-saved register was changed; use caller-saved registers such as rcx, r8-r11");
    free(big);
    return 0;
}
