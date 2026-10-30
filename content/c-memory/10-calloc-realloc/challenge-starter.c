#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>

typedef struct {
    int *data;
    size_t len;
    size_t cap;
} IntVec;

int vec_insert(IntVec *v, size_t i, int value) {
    return -1;
}

int vec_remove(IntVec *v, size_t i, int *out) {
    return -1;
}

// Optional: try it with Run.
int main(void) {
    printf("write vec_insert and vec_remove, then press Test\n");
    return 0;
}
