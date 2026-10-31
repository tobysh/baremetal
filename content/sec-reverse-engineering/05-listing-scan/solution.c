#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int scan(const int *a, int n) {
    int best = -1, bv = 0;
    for (int i = 0; i < n; i++) {
        if (a[i] > 0 && (best < 0 || a[i] < bv)) { best = i; bv = a[i]; }
    }
    return best;
}

int main(void) {
    int a[] = {5, 3, 8};
    printf("%d\n", scan(a, 3));
    return 0;
}
