static int same(int a[], int b[], int n) {
    for (int i = 0; i < n; i++) if (a[i] != b[i]) return 0;
    return 1;
}

int main(void) {
    int a[5] = {1, 2, 3, 4, 5};
    int ra[5] = {5, 4, 3, 2, 1};
    reverse_array(a, 5);
    CHECK("reverse_array reverses an odd length", same(a, ra, 5), "got %d %d %d %d %d", a[0], a[1], a[2], a[3], a[4]);
    int b[4] = {10, 20, 30, 40};
    int rb[4] = {40, 30, 20, 10};
    reverse_array(b, 4);
    CHECK("reverse_array reverses an even length", same(b, rb, 4), "got %d %d %d %d", b[0], b[1], b[2], b[3]);
    int c[5] = {1, 2, 3, 4, 5};
    int rc[5] = {3, 2, 1, 4, 5};
    reverse_array(c, 3);
    CHECK("reverse_array only touches the first n", same(c, rc, 5), "got %d %d %d %d %d", c[0], c[1], c[2], c[3], c[4]);
    int one[1] = {7};
    reverse_array(one, 1);
    reverse_array(one, 0);
    CHECK("reverse_array with length 0 or 1 changes nothing", one[0] == 7, "got %d", one[0]);

    int d[4] = {1, 2, 3, 4};
    int rd[4] = {2, 3, 4, 1};
    rotate_left(d, 4);
    CHECK("rotate_left moves the first element to the end", same(d, rd, 4), "got %d %d %d %d", d[0], d[1], d[2], d[3]);
    int e[3] = {-1, 0, 9};
    rotate_left(e, 3);
    rotate_left(e, 3);
    rotate_left(e, 3);
    CHECK("rotating n times restores the array", e[0] == -1 && e[1] == 0 && e[2] == 9, "got %d %d %d", e[0], e[1], e[2]);
    int f[4] = {1, 2, 3, 99};
    rotate_left(f, 3);
    CHECK("rotate_left leaves elements past n alone", f[0] == 2 && f[1] == 3 && f[2] == 1 && f[3] == 99, "got %d %d %d %d", f[0], f[1], f[2], f[3]);
    int g[1] = {5};
    rotate_left(g, 1);
    rotate_left(g, 0);
    CHECK("rotate_left with length 0 or 1 changes nothing", g[0] == 5, "got %d", g[0]);

    int h[6] = {9, 9, 9, 9, 9, 9};
    fill_squares(h, 6);
    int rh[6] = {0, 1, 4, 9, 16, 25};
    CHECK("fill_squares writes 0 1 4 9 16 25", same(h, rh, 6), "got %d %d %d %d %d %d", h[0], h[1], h[2], h[3], h[4], h[5]);
    int k[4] = {-1, -1, -1, -1};
    fill_squares(k, 2);
    CHECK("fill_squares only touches the first n", k[0] == 0 && k[1] == 1 && k[2] == -1, "got %d %d %d", k[0], k[1], k[2]);

    int m[7] = {3, 1, 3, 3, 2, 3, 4};
    int len = remove_value(m, 7, 3);
    CHECK("remove_value returns the new length", len == 3, "got %d", len);
    CHECK("remove_value keeps the others in order", m[0] == 1 && m[1] == 2 && m[2] == 4, "got %d %d %d", m[0], m[1], m[2]);
    int p[4] = {1, 2, 3, 4};
    CHECK("remove_value of a missing value keeps the length", remove_value(p, 4, 9) == 4 && p[3] == 4, "got %d", p[3]);
    int q[3] = {5, 5, 5};
    CHECK("removing everything gives length 0", remove_value(q, 3, 5) == 0, "expected 0");
    int r[5] = {1, 2, 3, 2, 2};
    len = remove_value(r, 3, 2);
    CHECK("remove_value respects n", len == 2 && r[0] == 1 && r[1] == 3, "got len %d", len);
    return 0;
}
