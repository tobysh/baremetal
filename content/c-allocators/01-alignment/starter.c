#include <stdio.h>
#include <stdint.h>
#include <stddef.h>

int is_pow2(size_t a) {
    return 0;
}

size_t align_up(size_t n, size_t a) {
    return 0;
}

size_t align_down(size_t n, size_t a) {
    return 0;
}

size_t padding_for(uintptr_t addr, size_t a) {
    return 0;
}

size_t layout(const size_t *size, const size_t *align, int n, size_t *offset) {
    return 0;
}

// Run it to compare with the compiler's own layout.
struct S { char c; int i; char d; double x; };

int main(void) {
    size_t sz[] = { sizeof(char), sizeof(int), sizeof(char), sizeof(double) };
    size_t al[] = { _Alignof(char), _Alignof(int), _Alignof(char), _Alignof(double) };
    size_t off[4];
    size_t total = layout(sz, al, 4, off);
    printf("offsets: %zu %zu %zu %zu, total %zu\n", off[0], off[1], off[2], off[3], total);
    printf("compiler: %zu %zu %zu %zu, total %zu\n", offsetof(struct S, c), offsetof(struct S, i),
           offsetof(struct S, d), offsetof(struct S, x), sizeof(struct S));
    printf("align_up(13, 8) = %zu (expected 16)\n", align_up(13, 8));
    return 0;
}
