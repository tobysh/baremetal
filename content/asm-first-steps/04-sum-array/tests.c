extern long sum_array(const long *a, size_t n);

int main(void) {
    long a[] = {1, 2, 3, 4, 5};
    long neg[] = {-1, -2, -3, 10};
    long one[] = {42};
    long big[] = {4000000000L, 4000000000L, -1L};
    long poison[1] = {999};

    CHECK("sum of {1,2,3,4,5} is 15", sum_array(a, 5) == 15, "got %ld", sum_array(a, 5));
    CHECK("empty array sums to 0", sum_array(poison, 0) == 0, "got %ld (n = 0 must not read the array)", sum_array(poison, 0));
    CHECK("single element", sum_array(one, 1) == 42, "got %ld", sum_array(one, 1));
    CHECK("negative numbers", sum_array(neg, 4) == 4, "got %ld", sum_array(neg, 4));
    CHECK("uses only the first n elements", sum_array(a, 3) == 6, "got %ld", sum_array(a, 3));
    CHECK("64-bit sums do not overflow early", sum_array(big, 3) == 7999999999L, "got %ld", sum_array(big, 3));

    long *many = malloc(1000 * sizeof *many);
    long expect = 0;
    for (long i = 0; i < 1000; i++) { many[i] = i * 3 - 500; expect += many[i]; }
    CHECK("1000 elements", sum_array(many, 1000) == expect, "got %ld, expected %ld", sum_array(many, 1000), expect);
    free(many);

    long sentinel[4] = {7, 8, 9, 10};
    sum_array(sentinel, 4);
    CHECK("the array is not modified", sentinel[0] == 7 && sentinel[1] == 8 && sentinel[2] == 9 && sentinel[3] == 10,
          "array is now {%ld, %ld, %ld, %ld}", sentinel[0], sentinel[1], sentinel[2], sentinel[3]);
    return 0;
}
