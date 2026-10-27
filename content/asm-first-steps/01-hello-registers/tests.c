extern long add_two(long a, long b);

int main(void) {
    CHECK("add_two(2, 3) is 5", add_two(2, 3) == 5, "got %ld", add_two(2, 3));
    CHECK("add_two(0, 0) is 0", add_two(0, 0) == 0, "got %ld", add_two(0, 0));
    CHECK("add_two(-4, 10) is 6", add_two(-4, 10) == 6, "got %ld", add_two(-4, 10));
    CHECK("add_two(-7, -8) is -15", add_two(-7, -8) == -15, "got %ld", add_two(-7, -8));
    CHECK("add_two works on 64-bit values", add_two(3000000000L, 4000000000L) == 7000000000L, "got %ld", add_two(3000000000L, 4000000000L));
    CHECK("add_two(a, b) equals add_two(b, a)", add_two(123, 456) == add_two(456, 123), "got %ld and %ld", add_two(123, 456), add_two(456, 123));
    return 0;
}
