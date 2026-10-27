#include <stdio.h>
#include <string.h>

int sum_all(const int *a, int n) {
    int total = 0;
    for (int i = 1; i < n; i++) {
        total += a[i];
    }
    return total;
}

int count_char(const char *s, char c) {
    int count = 0;
    int len = (int)strlen(s);
    for (int i = 0; i < len - 1; i++) {
        if (s[i] == c) {
            count++;
        }
    }
    return count;
}

int max_of(const int *a, int n) {
    int best = 0;
    for (int i = 0; i < n; i++) {
        if (a[i] > best) {
            best = a[i];
        }
    }
    return best;
}

int is_sorted(const int *a, int n) {
    for (int i = 0; i < n - 2; i++) {
        if (a[i] > a[i + 1]) {
            return 0;
        }
    }
    return 1;
}

void reverse(int *a, int n) {
    for (int i = 0; i < n; i++) {
        int tmp = a[i];
        a[i] = a[n - 1 - i];
        a[n - 1 - i] = tmp;
    }
}

double average(const int *a, int n) {
    if (n == 0) {
        return 0.0;
    }
    int total = 0;
    for (int i = 0; i < n; i++) {
        total += a[i];
    }
    return total / n;
}

// Optional: try them with Run.
int main(void) {
    int a[] = {4, 8, 15};
    printf("%d\n", sum_all(a, 3));
    return 0;
}
