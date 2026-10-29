#include <stdio.h>

void reverse_array(int a[], int n) {
    int left = 0;
    int right = n - 1;
    while (left < right) {
        int tmp = a[left];
        a[left] = a[right];
        a[right] = tmp;
        left++;
        right--;
    }
}

void rotate_left(int a[], int n) {
    if (n < 2) return;
    int first = a[0];
    for (int i = 0; i + 1 < n; i++) {
        a[i] = a[i + 1];
    }
    a[n - 1] = first;
}

void fill_squares(int a[], int n) {
    for (int i = 0; i < n; i++) {
        a[i] = i * i;
    }
}

int remove_value(int a[], int n, int v) {
    int keep = 0;
    for (int i = 0; i < n; i++) {
        if (a[i] != v) {
            a[keep] = a[i];
            keep++;
        }
    }
    return keep;
}

static void show(int a[], int n) {
    for (int i = 0; i < n; i++) printf("%d ", a[i]);
    printf("\n");
}

int main(void) {
    int a[5] = {1, 2, 3, 4, 5};
    reverse_array(a, 5);
    show(a, 5);
    rotate_left(a, 5);
    show(a, 5);
    return 0;
}
