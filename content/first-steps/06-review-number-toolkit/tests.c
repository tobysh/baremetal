int main(void) {
    CHECK("digit_sum(0) is 0", digit_sum(0) == 0, "got %d", digit_sum(0));
    CHECK("digit_sum(7) is 7", digit_sum(7) == 7, "got %d", digit_sum(7));
    CHECK("digit_sum(1234) is 10", digit_sum(1234) == 10, "got %d", digit_sum(1234));
    CHECK("digit_sum(1000) is 1", digit_sum(1000) == 1, "got %d", digit_sum(1000));
    CHECK("digit_sum(99999) is 45", digit_sum(99999) == 45, "got %d", digit_sum(99999));
    CHECK("digit_sum(2147483647) is 46", digit_sum(2147483647) == 46, "got %d", digit_sum(2147483647));

    CHECK("count_divisible(1, 10, 3) is 3", count_divisible(1, 10, 3) == 3, "got %d", count_divisible(1, 10, 3));
    CHECK("count_divisible(3, 9, 3) includes both ends", count_divisible(3, 9, 3) == 3, "got %d", count_divisible(3, 9, 3));
    CHECK("count_divisible(1, 1, 1) is 1", count_divisible(1, 1, 1) == 1, "got %d", count_divisible(1, 1, 1));
    CHECK("count_divisible(5, 5, 2) is 0", count_divisible(5, 5, 2) == 0, "got %d", count_divisible(5, 5, 2));
    CHECK("count_divisible(10, 1, 2) is 0 (empty range)", count_divisible(10, 1, 2) == 0, "got %d", count_divisible(10, 1, 2));
    CHECK("count_divisible(-4, 4, 2) counts negatives and zero", count_divisible(-4, 4, 2) == 5, "got %d", count_divisible(-4, 4, 2));
    CHECK("count_divisible(1, 100, 7) is 14", count_divisible(1, 100, 7) == 14, "got %d", count_divisible(1, 100, 7));

    CHECK("is_prime(0) is 0", is_prime(0) == 0, "got %d", is_prime(0));
    CHECK("is_prime(1) is 0", is_prime(1) == 0, "got %d", is_prime(1));
    CHECK("is_prime(-7) is 0", is_prime(-7) == 0, "got %d", is_prime(-7));
    CHECK("is_prime(2) is 1", is_prime(2) == 1, "got %d", is_prime(2));
    CHECK("is_prime(3) is 1", is_prime(3) == 1, "got %d", is_prime(3));
    CHECK("is_prime(4) is 0", is_prime(4) == 0, "got %d", is_prime(4));
    CHECK("is_prime(9) is 0", is_prime(9) == 0, "got %d", is_prime(9));
    CHECK("is_prime(97) is 1", is_prime(97) == 1, "got %d", is_prime(97));
    CHECK("is_prime(100) is 0", is_prime(100) == 0, "got %d", is_prime(100));
    CHECK("is_prime(7919) is 1", is_prime(7919) == 1, "got %d", is_prime(7919));
    return 0;
}
