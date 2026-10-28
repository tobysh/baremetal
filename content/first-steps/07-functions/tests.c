int main(void) {
    CHECK("add(2, 3) is 5", add(2, 3) == 5, "got %d", add(2, 3));
    CHECK("add(-4, 10) is 6", add(-4, 10) == 6, "got %d", add(-4, 10));
    CHECK("add(0, 0) is 0", add(0, 0) == 0, "got %d", add(0, 0));
    CHECK("square(4) is 16", square(4) == 16, "got %d", square(4));
    CHECK("square(-3) is 9", square(-3) == 9, "got %d", square(-3));
    CHECK("square(0) is 0", square(0) == 0, "got %d", square(0));
    return 0;
}
