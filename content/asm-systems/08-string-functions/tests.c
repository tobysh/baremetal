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

#include <sys/mman.h>
#include <unistd.h>
extern size_t my_strlen(const char *s);
extern void *my_memcpy(void *dst, const void *src, size_t n);
extern int my_strcmp(const char *a, const char *b);

/* Returns a pointer to a copy of s whose terminator is the last accessible byte
   of a page; the following page is inaccessible. */
static char *edge(const char *s) {
    long pg = sysconf(_SC_PAGESIZE);
    char *m = mmap(NULL, 3 * pg, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    mprotect(m + 2 * pg, pg, PROT_NONE);
    size_t n = strlen(s) + 1;
    char *p = m + 2 * pg - n;
    memcpy(p, s, n);
    return p;
}
static char *edge_buf(size_t n) { /* n writable bytes flush against the guard page */
    long pg = sysconf(_SC_PAGESIZE);
    char *m = mmap(NULL, 3 * pg, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    mprotect(m + 2 * pg, pg, PROT_NONE);
    return m + 2 * pg - n;
}
static int sgn(int x) { return (x > 0) - (x < 0); }

int main(void) {
    size_t n;
    n = my_strlen("");                 CHECK("strlen of the empty string", n == 0, "got %zu", n);
    n = my_strlen("a");                CHECK("strlen of one character", n == 1, "got %zu", n);
    n = my_strlen("hello, world");     CHECK("strlen of a sentence", n == 12, "got %zu", n);
    n = my_strlen("tab\there\n");      CHECK("strlen counts control characters", n == 9, "got %zu", n);
    n = my_strlen("caf\xc3\xa9");      CHECK("strlen counts bytes above 127", n == 5, "got %zu", n);
    n = my_strlen(edge("flush"));      CHECK("strlen does not read past the terminator", n == 5, "got %zu", n);
    n = my_strlen(edge(""));           CHECK("empty string at the page edge", n == 0, "got %zu", n);

    char dst[32]; memset(dst, 'X', sizeof dst);
    void *r = my_memcpy(dst, "abcdef", 6);
    CHECK("memcpy copies bytes and returns dst", r == dst && memcmp(dst, "abcdef", 6) == 0 && dst[6] == 'X', "returned %p (dst is %p), text \"%.8s\"", r, (void *)dst, dst);
    memset(dst, 'X', sizeof dst);
    r = my_memcpy(dst + 3, "hi", 0);
    CHECK("memcpy with n = 0 copies nothing", r == dst + 3 && dst[3] == 'X', "returned %p, dst[3] = %c", r, dst[3]);
    unsigned char src[300], out[300];
    for (int i = 0; i < 300; i++) src[i] = (unsigned char)(i * 7 + 3);
    memset(out, 0, sizeof out);
    my_memcpy(out, src, 300);
    CHECK("memcpy of 300 binary bytes", memcmp(out, src, 300) == 0, "contents differ");
    char *e = edge_buf(5);
    my_memcpy(e, "12345", 5);
    CHECK("memcpy writes exactly n bytes", memcmp(e, "12345", 5) == 0, "contents differ");
    e = edge("exact");
    char *e2 = edge_buf(6);
    my_memcpy(e2, e, 6);
    CHECK("memcpy reads exactly n bytes", strcmp(e2, "exact") == 0, "contents differ");

    int c;
    c = my_strcmp("abc", "abc");   CHECK("equal strings", c == 0, "got %d", c);
    c = my_strcmp("", "");         CHECK("two empty strings", c == 0, "got %d", c);
    c = my_strcmp("abc", "abd");   CHECK("abc < abd", c < 0, "got %d", c);
    c = my_strcmp("abd", "abc");   CHECK("abd > abc", c > 0, "got %d", c);
    c = my_strcmp("ab", "abc");    CHECK("a prefix is smaller", c < 0, "got %d", c);
    c = my_strcmp("abc", "ab");    CHECK("longer is larger", c > 0, "got %d", c);
    c = my_strcmp("", "a");        CHECK("empty < non-empty", c < 0, "got %d", c);
    c = my_strcmp("\xc3\xa9", "A");CHECK("bytes above 127 compare as unsigned", c > 0, "got %d (treated 0xC3 as negative?)", c);
    c = my_strcmp(edge("same"), edge("same")); CHECK("equal strings at the page edge", c == 0, "got %d", c);
    c = my_strcmp(edge("a"), edge("b"));       CHECK("stops at the first difference", c < 0, "got %d", c);
    const char *w[] = {"", "a", "ab", "abc", "b", "\x7f", "\x80", "\xff", "zz"};
    int ok = 1; int i = 0, j = 0;
    for (i = 0; i < 9 && ok; i++) for (j = 0; j < 9; j++) if (sgn(my_strcmp(w[i], w[j])) != sgn(strcmp(w[i], w[j]))) { ok = 0; break; }
    CHECK("agrees with libc on every pair of 9 strings", ok, "wrong sign for \"%s\" vs \"%s\"", w[i - 1], w[j]);

    PRESERVED("my_strlen preserves callee-saved registers", my_strlen, "abc", 0, 0);
    PRESERVED("my_memcpy preserves callee-saved registers", my_memcpy, dst, "xy", 2);
    PRESERVED("my_strcmp preserves callee-saved registers", my_strcmp, "a", "b", 0);
    return 0;
}
