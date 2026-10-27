extern long abs_val(long x);

int main(void) {
    CHECK("abs_val(5) is 5", abs_val(5) == 5, "got %ld", abs_val(5));
    CHECK("abs_val(-5) is 5", abs_val(-5) == 5, "got %ld", abs_val(-5));
    CHECK("abs_val(0) is 0", abs_val(0) == 0, "got %ld", abs_val(0));
    CHECK("abs_val(1) is 1", abs_val(1) == 1, "got %ld", abs_val(1));
    CHECK("abs_val(-1) is 1", abs_val(-1) == 1, "got %ld", abs_val(-1));
    CHECK("abs_val(-123456789012) is 123456789012", abs_val(-123456789012L) == 123456789012L, "got %ld", abs_val(-123456789012L));
    CHECK("abs_val(9000000000) is 9000000000", abs_val(9000000000L) == 9000000000L, "got %ld", abs_val(9000000000L));
    CHECK("abs_val(-LONG_MAX) is LONG_MAX", abs_val(-9223372036854775807L) == 9223372036854775807L, "got %ld", abs_val(-9223372036854775807L));
    return 0;
}
