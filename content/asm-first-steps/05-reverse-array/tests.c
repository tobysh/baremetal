extern void reverse_array(long *a, size_t n);

static int same(const long *a, const long *b, size_t n) {
    for (size_t i = 0; i < n; i++) if (a[i] != b[i]) return 0;
    return 1;
}

int main(void) {
    long even[] = {1, 2, 3, 4, 5, 6};
    long even_r[] = {6, 5, 4, 3, 2, 1};
    reverse_array(even, 6);
    CHECK("reverses an even-length array", same(even, even_r, 6), "got {%ld, %ld, %ld, %ld, %ld, %ld}", even[0], even[1], even[2], even[3], even[4], even[5]);

    long odd[] = {10, 20, 30, 40, 50};
    long odd_r[] = {50, 40, 30, 20, 10};
    reverse_array(odd, 5);
    CHECK("reverses an odd-length array (middle stays)", same(odd, odd_r, 5), "got {%ld, %ld, %ld, %ld, %ld}", odd[0], odd[1], odd[2], odd[3], odd[4]);

    long two[] = {-1, 1};
    reverse_array(two, 2);
    CHECK("two elements swap", two[0] == 1 && two[1] == -1, "got {%ld, %ld}", two[0], two[1]);

    long one[] = {77};
    reverse_array(one, 1);
    CHECK("one element is unchanged", one[0] == 77, "got %ld", one[0]);

    long zero[] = {5, 6, 7};
    reverse_array(zero, 0);
    CHECK("n = 0 changes nothing", zero[0] == 5 && zero[1] == 6 && zero[2] == 7, "got {%ld, %ld, %ld}", zero[0], zero[1], zero[2]);

    // Guard values on both sides catch writes outside the array.
    long guard[9] = {111, 222, 1, 2, 3, 4, 5, 333, 444};
    reverse_array(guard + 2, 5);
    CHECK("elements are reversed inside a larger buffer", guard[2] == 5 && guard[3] == 4 && guard[4] == 3 && guard[5] == 2 && guard[6] == 1,
          "got {%ld, %ld, %ld, %ld, %ld}", guard[2], guard[3], guard[4], guard[5], guard[6]);
    CHECK("does not write outside the array", guard[0] == 111 && guard[1] == 222 && guard[7] == 333 && guard[8] == 444,
          "neighbours are now {%ld, %ld} and {%ld, %ld}", guard[0], guard[1], guard[7], guard[8]);

    long *big = malloc(1001 * sizeof *big);
    for (long i = 0; i < 1001; i++) big[i] = i * i - 7;
    reverse_array(big, 1001);
    int ok = 1;
    for (long i = 0; i < 1001; i++) { long j = 1000 - i; if (big[i] != j * j - 7) ok = 0; }
    CHECK("1001 elements", ok, "a large array was not reversed correctly");
    reverse_array(big, 1001);
    ok = 1;
    for (long i = 0; i < 1001; i++) if (big[i] != i * i - 7) ok = 0;
    CHECK("reversing twice restores the original", ok, "second reversal did not restore the array");
    free(big);
    return 0;
}
