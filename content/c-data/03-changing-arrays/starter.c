#include <stdio.h>

void reverse_array(int a[], int n) {
}

void rotate_left(int a[], int n) {
}

void fill_squares(int a[], int n) {
}

int remove_value(int a[], int n, int v) {
    return n;
}

static void show(int a[], int n) {
    for (int i = 0; i < n; i++) printf("%d ", a[i]);
    printf("\n");
}

// Optional: try them with Run.
int main(void) {
    int a[5] = {1, 2, 3, 4, 5};
    reverse_array(a, 5);
    show(a, 5);              // expected 5 4 3 2 1
    rotate_left(a, 5);
    show(a, 5);              // expected 4 3 2 1 5
    return 0;
}
