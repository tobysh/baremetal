int main(void) {
    int x = 1, y = 2;
    swap(&x, &y);
    CHECK("swap(&x, &y) with 1 and 2 makes x = 2", x == 2, "x was %d", x);
    CHECK("swap(&x, &y) with 1 and 2 makes y = 1", y == 1, "y was %d", y);

    int a = -7, b = 100;
    swap(&a, &b);
    CHECK("swap handles negative numbers (a = 100, b = -7)", a == 100 && b == -7, "a = %d, b = %d", a, b);

    int z = 0, w = 55;
    swap(&z, &w);
    CHECK("swap handles zero (z = 55, w = 0)", z == 55 && w == 0, "z = %d, w = %d", z, w);

    int s = 9, t = 9;
    swap(&s, &t);
    CHECK("swapping equal values leaves both at 9", s == 9 && t == 9, "s = %d, t = %d", s, t);

    int p = 3, q = 4;
    swap(&p, &q);
    swap(&p, &q);
    CHECK("swapping twice restores the original values", p == 3 && q == 4, "p = %d, q = %d", p, q);

    int arr[3] = {10, 20, 30};
    swap(&arr[0], &arr[2]);
    CHECK("swap works on array elements ({10,20,30} becomes {30,20,10})", arr[0] == 30 && arr[1] == 20 && arr[2] == 10, "got {%d,%d,%d}", arr[0], arr[1], arr[2]);
    return 0;
}
