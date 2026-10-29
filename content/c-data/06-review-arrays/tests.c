int main(void) {
    int a[5] = {3, 1, 4, 1, 5};
    int out[5] = {0, 0, 0, 0, 0};
    prefix_sums(a, 5, out);
    CHECK("prefix_sums of 3 1 4 1 5", out[0] == 3 && out[1] == 4 && out[2] == 8 && out[3] == 9 && out[4] == 14, "got %d %d %d %d %d", out[0], out[1], out[2], out[3], out[4]);
    int b[4] = {-2, 5, -10, 4};
    int o2[4];
    prefix_sums(b, 4, o2);
    CHECK("prefix_sums with negatives", o2[0] == -2 && o2[1] == 3 && o2[2] == -7 && o2[3] == -3, "got %d %d %d %d", o2[0], o2[1], o2[2], o2[3]);
    CHECK("prefix_sums leaves the input alone", b[0] == -2 && b[3] == 4, "input changed");
    int o3[3] = {77, 77, 77};
    prefix_sums(a, 2, o3);
    CHECK("prefix_sums writes only n values", o3[0] == 3 && o3[1] == 4 && o3[2] == 77, "got %d %d %d", o3[0], o3[1], o3[2]);

    CHECK("second_largest of 3 1 4 1 5 is 4", second_largest(a, 5) == 4, "got %d", second_largest(a, 5));
    int c[4] = {5, 9, 9, 3};
    CHECK("second_largest skips a repeated maximum", second_largest(c, 4) == 5, "got %d", second_largest(c, 4));
    int d[3] = {-8, -3, -5};
    CHECK("second_largest with negatives", second_largest(d, 3) == -5, "got %d", second_largest(d, 3));
    int e[2] = {1, 2};
    CHECK("second_largest of two values", second_largest(e, 2) == 1, "got %d", second_largest(e, 2));
    int f[5] = {7, 7, 2, 7, 7};
    CHECK("second_largest among many repeats", second_largest(f, 5) == 2, "got %d", second_largest(f, 5));
    int g[3] = {-1, 0, -1};
    CHECK("second_largest is -1 here, not 0", second_largest(g, 3) == -1, "got %d", second_largest(g, 3));

    int x[4] = {1, 2, 3, 4};
    int y[4] = {1, 2, 3, 4};
    int w[4] = {1, 2, 3, 5};
    CHECK("arrays_equal for identical arrays", arrays_equal(x, y, 4) == 1, "got %d", arrays_equal(x, y, 4));
    CHECK("arrays_equal notices a last-element difference", arrays_equal(x, w, 4) == 0, "got %d", arrays_equal(x, w, 4));
    CHECK("arrays_equal looks at the first n only", arrays_equal(x, w, 3) == 1, "got %d", arrays_equal(x, w, 3));
    CHECK("arrays_equal of length 0 is 1", arrays_equal(x, w, 0) == 1, "got %d", arrays_equal(x, w, 0));

    CHECK("count_peaks of 3 1 4 1 5 is 1", count_peaks(a, 5) == 1, "got %d", count_peaks(a, 5));
    int p[7] = {1, 3, 1, 5, 2, 8, 0};
    CHECK("count_peaks finds three peaks", count_peaks(p, 7) == 3, "got %d", count_peaks(p, 7));
    int q[4] = {9, 1, 1, 9};
    CHECK("the ends are never peaks", count_peaks(q, 4) == 0, "got %d", count_peaks(q, 4));
    int r[5] = {1, 2, 2, 2, 1};
    CHECK("a plateau is not a peak", count_peaks(r, 5) == 0, "got %d", count_peaks(r, 5));
    CHECK("fewer than 3 elements means 0 peaks", count_peaks(p, 2) == 0 && count_peaks(p, 0) == 0, "expected 0");
    return 0;
}
