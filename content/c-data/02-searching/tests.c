int main(void) {
    int a[6] = {4, 8, 15, 8, 23, 8};
    CHECK("index_of finds the first 8 at index 1", index_of(a, 6, 8) == 1, "got %d", index_of(a, 6, 8));
    CHECK("index_of finds the first element", index_of(a, 6, 4) == 0, "got %d", index_of(a, 6, 4));
    CHECK("index_of finds the last element", index_of(a, 6, 23) == 4 && index_of(a, 5, 15) == 2, "got %d", index_of(a, 6, 23));
    CHECK("index_of returns -1 when missing", index_of(a, 6, 99) == -1, "got %d", index_of(a, 6, 99));
    CHECK("index_of only searches the first n elements", index_of(a, 3, 23) == -1, "got %d", index_of(a, 3, 23));
    CHECK("index_of on an empty array is -1", index_of(a, 0, 4) == -1, "got %d", index_of(a, 0, 4));

    CHECK("last_index_of finds the last 8 at index 5", last_index_of(a, 6, 8) == 5, "got %d", last_index_of(a, 6, 8));
    CHECK("last_index_of respects n", last_index_of(a, 4, 8) == 3, "got %d", last_index_of(a, 4, 8));
    CHECK("last_index_of for a single match", last_index_of(a, 6, 15) == 2, "got %d", last_index_of(a, 6, 15));
    CHECK("last_index_of returns -1 when missing", last_index_of(a, 6, -5) == -1, "got %d", last_index_of(a, 6, -5));
    CHECK("last_index_of on an empty array is -1", last_index_of(a, 0, 4) == -1, "got %d", last_index_of(a, 0, 4));

    CHECK("count_value counts three 8s", count_value(a, 6, 8) == 3, "got %d", count_value(a, 6, 8));
    CHECK("count_value of a missing value is 0", count_value(a, 6, 7) == 0, "got %d", count_value(a, 6, 7));
    int z[4] = {0, 0, 0, 0};
    CHECK("count_value counts zeros", count_value(z, 4, 0) == 4, "got %d", count_value(z, 4, 0));
    CHECK("count_value respects n", count_value(a, 2, 8) == 1, "got %d", count_value(a, 2, 8));

    int s1[5] = {1, 2, 2, 7, 9};
    int s2[5] = {1, 2, 3, 9, 8};
    int s3[3] = {5, 4, 3};
    CHECK("is_sorted accepts repeated values", is_sorted(s1, 5) == 1, "got %d", is_sorted(s1, 5));
    CHECK("is_sorted rejects a drop at the end", is_sorted(s2, 5) == 0, "got %d", is_sorted(s2, 5));
    CHECK("is_sorted rejects descending data", is_sorted(s3, 3) == 0, "got %d", is_sorted(s3, 3));
    CHECK("is_sorted ignores elements past n", is_sorted(s2, 4) == 1, "got %d", is_sorted(s2, 4));
    CHECK("is_sorted of one element is 1", is_sorted(s3, 1) == 1, "got %d", is_sorted(s3, 1));
    CHECK("is_sorted of an empty array is 1", is_sorted(s3, 0) == 1, "got %d", is_sorted(s3, 0));
    return 0;
}
