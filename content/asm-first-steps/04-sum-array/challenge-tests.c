extern long max_array(const long *a, size_t n);

int main(void) {
    long a[] = {3, 9, 2, 7};
    long neg[] = {-8, -3, -20, -5};
    long one[] = {-42};
    long first[] = {100, 1, 2, 3};
    long last[] = {1, 2, 3, 100};
    long big[] = {4000000000L, -4000000000L, 5000000000L, 1};
    long extremes[] = {-9223372036854775807L - 1, 0, 9223372036854775807L};

    CHECK("max of {3,9,2,7} is 9", max_array(a, 4) == 9, "got %ld", max_array(a, 4));
    CHECK("all negative: the answer is the least negative", max_array(neg, 4) == -3, "got %ld (compare as signed, and don't start from 0)", max_array(neg, 4));
    CHECK("single element", max_array(one, 1) == -42, "got %ld", max_array(one, 1));
    CHECK("maximum is the first element", max_array(first, 4) == 100, "got %ld", max_array(first, 4));
    CHECK("maximum is the last element", max_array(last, 4) == 100, "got %ld", max_array(last, 4));
    CHECK("64-bit values", max_array(big, 4) == 5000000000L, "got %ld", max_array(big, 4));
    CHECK("full signed range", max_array(extremes, 3) == 9223372036854775807L, "got %ld", max_array(extremes, 3));
    CHECK("uses only the first n elements", max_array(last, 3) == 3, "got %ld", max_array(last, 3));
    CHECK("the array is not modified", last[3] == 100 && last[0] == 1, "array contents changed");
    return 0;
}
