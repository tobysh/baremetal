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
extern void *my_memmove(void *dst, const void *src, size_t n);
extern char *my_strchr(const char *s, int c);

/* A buffer of n bytes with an inaccessible page directly before and after it. */
static unsigned char *fenced(size_t n) {
    long pg = sysconf(_SC_PAGESIZE);
    unsigned char *m = mmap(NULL, 4 * pg, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    mprotect(m, pg, PROT_NONE);
    mprotect(m + 3 * pg, pg, PROT_NONE);
    return m + 2 * pg - n % (size_t)pg;      /* ends exactly at the page boundary */
}
static int df_set(void) {
    unsigned long f;
    __asm__ volatile("pushfq; pop %0" : "=r"(f));
    return (f & 0x400) != 0;
}

int main(void) {
    unsigned char *b = fenced(64);
    unsigned char ref[64];
    int ok;

    for (int i = 0; i < 64; i++) b[i] = (unsigned char)(i + 1);
    void *r = my_memmove(b + 20, b + 5, 30);   /* dst after src, overlapping */
    for (int i = 0; i < 64; i++) ref[i] = (unsigned char)(i + 1);
    memmove(ref + 20, ref + 5, 30);
    CHECK("overlap with dst above src copies backwards", memcmp(b, ref, 64) == 0 && r == b + 20, "contents differ from libc memmove (copying forwards smears the data)");
    CHECK("the direction flag is clear after a backward copy", !df_set(), "DF is still set: add cld before ret");

    for (int i = 0; i < 64; i++) b[i] = (unsigned char)(i + 1);
    r = my_memmove(b + 3, b + 18, 40);        /* dst before src, overlapping */
    for (int i = 0; i < 64; i++) ref[i] = (unsigned char)(i + 1);
    memmove(ref + 3, ref + 18, 40);
    CHECK("overlap with dst below src copies forwards", memcmp(b, ref, 64) == 0 && r == b + 3, "contents differ from libc memmove");

    for (int i = 0; i < 64; i++) b[i] = (unsigned char)(i + 1);
    r = my_memmove(b + 1, b, 63);              /* shift everything up by one, ends flush against the guard */
    for (int i = 0; i < 64; i++) ref[i] = (unsigned char)(i + 1);
    memmove(ref + 1, ref, 63);
    CHECK("shifting a whole buffer up by one byte", memcmp(b, ref, 64) == 0 && r == b + 1, "contents differ from libc memmove");

    for (int i = 0; i < 64; i++) b[i] = (unsigned char)(i + 1);
    r = my_memmove(b, b + 1, 63);              /* shift down by one, starts flush against the other guard if n == 64 */
    for (int i = 0; i < 64; i++) ref[i] = (unsigned char)(i + 1);
    memmove(ref, ref + 1, 63);
    CHECK("shifting a whole buffer down by one byte", memcmp(b, ref, 64) == 0 && r == b, "contents differ from libc memmove");

    for (int i = 0; i < 64; i++) b[i] = (unsigned char)(i + 1);
    r = my_memmove(b + 10, b + 10, 20);
    CHECK("dst == src changes nothing", r == b + 10 && b[10] == 11 && b[29] == 30, "contents changed");

    unsigned char dst[16], src[16];
    memset(dst, 0xAA, sizeof dst);
    for (int i = 0; i < 16; i++) src[i] = (unsigned char)(0x40 + i);
    r = my_memmove(dst + 4, src, 8);
    ok = r == dst + 4 && dst[3] == 0xAA && dst[4] == 0x40 && dst[11] == 0x47 && dst[12] == 0xAA;
    CHECK("separate buffers: copies exactly n bytes", ok, "wrote outside the n bytes or copied the wrong ones");
    memset(dst, 0xAA, sizeof dst);
    r = my_memmove(dst, src, 0);
    CHECK("n = 0 copies nothing and returns dst", r == dst && dst[0] == 0xAA, "returned %p", r);
    CHECK("the direction flag is still clear", !df_set(), "DF is set");

    /* exhaustive overlap check: every offset pair in a small window */
    int bad = 0;
    for (int d = 0; d < 12; d++)
        for (int s = 0; s < 12; s++)
            for (int n = 0; n <= 12; n++) {
                unsigned char x[32], y[32];
                for (int i = 0; i < 32; i++) x[i] = y[i] = (unsigned char)(i * 7 + 1);
                my_memmove(x + d, x + s, (size_t)n);
                memmove(y + d, y + s, (size_t)n);
                if (memcmp(x, y, 32) != 0) bad++;
            }
    CHECK("every overlap offset and length from 0 to 11 agrees with libc", bad == 0, "%d of %d combinations differ", bad, 12 * 12 * 13);

    char *p;
    const char *hay = "find the needle";
    char *e = (char *)fenced(32) + 32 - 16;
    memcpy(e, hay, strlen(hay) + 1);
    p = my_strchr(e, 'n');
    CHECK("finds the first match", p == e + 2, "returned offset %ld, expected 2", p ? (long)(p - e) : -1L);
    p = my_strchr(e, 'e');
    CHECK("first of several matches", p == e + 7, "returned offset %ld, expected 7", p ? (long)(p - e) : -1L);
    p = my_strchr(e, 'z');
    CHECK("a missing byte gives NULL without reading past the terminator", p == NULL, "returned %p", (void *)p);
    p = my_strchr(e, 0);
    CHECK("searching for 0 returns the terminator", p == e + strlen(hay), "returned offset %ld, expected %zu", p ? (long)(p - e) : -1L, strlen(hay));
    p = my_strchr(e, 'f');
    CHECK("a match at the very first byte", p == e, "returned offset %ld", p ? (long)(p - e) : -1L);

    char *w = (char *)fenced(8) + 8 - 4;
    memcpy(w, "abc", 4);
    p = my_strchr(w, 'c');
    CHECK("a match just before the page edge", p == w + 2, "returned offset %ld, expected 2", p ? (long)(p - w) : -1L);
    p = my_strchr(w, 'd');
    CHECK("no match in a string that ends at the page edge", p == NULL, "returned %p", (void *)p);

    char *emp = (char *)fenced(1) + 1 - 1;
    emp[0] = 0;
    p = my_strchr(emp, 'a');
    CHECK("the empty string has no 'a'", p == NULL, "returned %p", (void *)p);
    p = my_strchr(emp, 0);
    CHECK("the empty string's terminator is found at offset 0", p == emp, "returned %p", (void *)p);

    char hi[8] = { 'a', (char)0xE9, 'b', (char)0x7F, 'c', 0 };
    p = my_strchr(hi, 0xE9);
    CHECK("bytes above 127 are matched as unsigned values", p == hi + 1, "returned offset %ld, expected 1", p ? (long)(p - hi) : -1L);
    p = my_strchr(hi, 0x1E9);
    CHECK("only the low 8 bits of c count", p == hi + 1, "returned offset %ld, expected 1", p ? (long)(p - hi) : -1L);
    p = my_strchr(hi, -23);
    CHECK("a negative int is the byte 0xE9 too", p == hi + 1, "returned offset %ld, expected 1", p ? (long)(p - hi) : -1L);
    p = my_strchr(hi, 0x100);
    CHECK("0x100 is the byte 0, i.e. the terminator", p == hi + 5, "returned offset %ld, expected 5", p ? (long)(p - hi) : -1L);

    PRESERVED("my_strchr preserves callee-saved registers", my_strchr, hay, 'e', 0);
    PRESERVED("my_memmove preserves callee-saved registers", my_memmove, dst, src, 4);
    return 0;
}
