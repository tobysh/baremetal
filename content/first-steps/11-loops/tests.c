int main(void) {
    CHECK("sum_to(1) is 1", sum_to(1) == 1, "got %d", sum_to(1));
    CHECK("sum_to(4) is 10", sum_to(4) == 10, "got %d", sum_to(4));
    CHECK("sum_to(100) is 5050", sum_to(100) == 5050, "got %d", sum_to(100));
    CHECK("sum_to(0) is 0", sum_to(0) == 0, "got %d", sum_to(0));
    CHECK("sum_to(-5) is 0", sum_to(-5) == 0, "got %d", sum_to(-5));
    CHECK("factorial(0) is 1", factorial(0) == 1, "got %ld", factorial(0));
    CHECK("factorial(1) is 1", factorial(1) == 1, "got %ld", factorial(1));
    CHECK("factorial(5) is 120", factorial(5) == 120, "got %ld", factorial(5));
    CHECK("factorial(15) is 1307674368000", factorial(15) == 1307674368000L, "got %ld", factorial(15));
    return 0;
}
