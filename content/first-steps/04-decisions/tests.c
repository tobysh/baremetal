int main(void) {
    CHECK("max_of_three(1, 2, 3) is 3", max_of_three(1, 2, 3) == 3, "got %d", max_of_three(1, 2, 3));
    CHECK("max_of_three(9, 2, 3) is 9", max_of_three(9, 2, 3) == 9, "got %d", max_of_three(9, 2, 3));
    CHECK("max_of_three(1, 8, 3) is 8", max_of_three(1, 8, 3) == 8, "got %d", max_of_three(1, 8, 3));
    CHECK("max_of_three(-5, -2, -9) is -2", max_of_three(-5, -2, -9) == -2, "got %d", max_of_three(-5, -2, -9));
    CHECK("max_of_three(4, 4, 4) is 4", max_of_three(4, 4, 4) == 4, "got %d", max_of_three(4, 4, 4));
    CHECK("is_even(4) is 1", is_even(4) == 1, "got %d", is_even(4));
    CHECK("is_even(7) is 0", is_even(7) == 0, "got %d", is_even(7));
    CHECK("is_even(0) is 1", is_even(0) == 1, "got %d", is_even(0));
    CHECK("is_even(-6) is 1", is_even(-6) == 1, "got %d", is_even(-6));
    CHECK("is_even(-3) is 0", is_even(-3) == 0, "got %d", is_even(-3));
    return 0;
}
