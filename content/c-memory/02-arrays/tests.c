int main(void) {
    int a[] = {3, -1, 7, 2};
    int one[] = {42};
    int neg[] = {-8, -3, -20, -5};
    int big[100];
    for (int i = 0; i < 100; i++) big[i] = i + 1;
    int dup[] = {5, 5, 5};
    int last[] = {1, 2, 3, 99};
    int first[] = {99, 2, 3, 1};
    int empty[1] = {123};

    CHECK("array_sum({3, -1, 7, 2}, 4) is 11", array_sum(a, 4) == 11, "got %d", array_sum(a, 4));
    CHECK("array_sum({42}, 1) is 42", array_sum(one, 1) == 42, "got %d", array_sum(one, 1));
    CHECK("array_sum of 1..100 is 5050", array_sum(big, 100) == 5050, "got %d", array_sum(big, 100));
    CHECK("array_sum({-8, -3, -20, -5}, 4) is -36", array_sum(neg, 4) == -36, "got %d", array_sum(neg, 4));
    CHECK("array_sum with len 0 is 0 (reads nothing)", array_sum(empty, 0) == 0, "got %d", array_sum(empty, 0));
    CHECK("array_sum only adds the first len elements", array_sum(a, 2) == 2, "got %d", array_sum(a, 2));

    CHECK("array_max({3, -1, 7, 2}, 4) is 7", array_max(a, 4) == 7, "got %d", array_max(a, 4));
    CHECK("array_max({42}, 1) is 42", array_max(one, 1) == 42, "got %d", array_max(one, 1));
    CHECK("array_max of all negatives {-8, -3, -20, -5} is -3", array_max(neg, 4) == -3, "got %d", array_max(neg, 4));
    CHECK("array_max finds the max in the last position", array_max(last, 4) == 99, "got %d", array_max(last, 4));
    CHECK("array_max finds the max in the first position", array_max(first, 4) == 99, "got %d", array_max(first, 4));
    CHECK("array_max({5, 5, 5}, 3) is 5", array_max(dup, 3) == 5, "got %d", array_max(dup, 3));
    CHECK("array_max only looks at the first len elements", array_max(last, 3) == 3, "got %d", array_max(last, 3));
    return 0;
}
