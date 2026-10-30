#include <stdio.h>
#include <stddef.h>

int sum_between(const int *begin, const int *end) {
    int total = 0;
    for (const int *p = begin; p != end; p++) {
        total += *p;
    }
    return total;
}

const int *find_max(const int *arr, size_t n) {
    if (n == 0) {
        return NULL;
    }
    const int *best = arr;
    for (const int *p = arr + 1; p < arr + n; p++) {
        if (*p > *best) {
            best = p;
        }
    }
    return best;
}

ptrdiff_t gap(const int *a, const int *b) {
    return b - a;
}

int main(void) {
    int a[] = {4, 9, 2, 7, 5};
    printf("sum_between(a, a + 5) = %d\n", sum_between(a, a + 5));
    const int *m = find_max(a, 5);
    printf("find_max index = %d\n", m ? (int)(m - a) : -1);
    printf("gap(a, a + 3) = %td\n", gap(a, a + 3));
    return 0;
}
