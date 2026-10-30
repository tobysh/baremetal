#include <stdio.h>
#include <stddef.h>

int sum_between(const int *begin, const int *end) {
    return 0;
}

const int *find_max(const int *arr, size_t n) {
    return NULL;
}

ptrdiff_t gap(const int *a, const int *b) {
    return 0;
}

// Optional: try them with Run.
int main(void) {
    int a[] = {4, 9, 2, 7, 5};
    printf("sum_between(a, a + 5) = %d (expected 27)\n", sum_between(a, a + 5));
    const int *m = find_max(a, 5);
    printf("find_max index = %d (expected 1)\n", m ? (int)(m - a) : -1);
    printf("gap(a, a + 3) = %td (expected 3)\n", gap(a, a + 3));
    return 0;
}
