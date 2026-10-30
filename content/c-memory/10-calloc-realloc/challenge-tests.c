static int vec_is(const IntVec *v, const int *want, size_t n) {
    if (v->len != n) return 0;
    for (size_t i = 0; i < n; i++) if (v->data[i] != want[i]) return 0;
    return 1;
}

int main(void) {
    IntVec v = {NULL, 0, 0};
    int r = vec_insert(&v, 0, 10);
    CHECK("inserting into an empty vector works and gives capacity 4", r == 0 && v.len == 1 && v.cap == 4 && v.data[0] == 10, "r = %d, len = %zu, cap = %zu", r, v.len, v.cap);

    vec_insert(&v, 1, 30);
    vec_insert(&v, 1, 20);
    vec_insert(&v, 0, 5);
    int w1[] = {5, 10, 20, 30};
    CHECK("inserts at the front, middle and end shift later items", vec_is(&v, w1, 4) && v.cap == 4, "len = %zu, cap = %zu", v.len, v.cap);

    r = vec_insert(&v, 2, 15);
    int w2[] = {5, 10, 15, 20, 30};
    CHECK("a middle insert into a full vector doubles capacity to 8", r == 0 && vec_is(&v, w2, 5) && v.cap == 8, "r = %d, len = %zu, cap = %zu", r, v.len, v.cap);

    r = vec_insert(&v, 7, 99);
    CHECK("an index past len is rejected and changes nothing", r == -1 && vec_is(&v, w2, 5) && v.cap == 8, "r = %d, len = %zu", r, v.len);

    int out = -1;
    r = vec_remove(&v, 2, &out);
    int w3[] = {5, 10, 20, 30};
    CHECK("remove from the middle returns the value and closes the gap", r == 0 && out == 15 && vec_is(&v, w3, 4), "r = %d, out = %d, len = %zu", r, out, v.len);

    r = vec_remove(&v, 0, NULL);
    int w4[] = {10, 20, 30};
    CHECK("remove accepts a NULL out pointer", r == 0 && vec_is(&v, w4, 3), "r = %d, len = %zu", r, v.len);

    out = -1;
    r = vec_remove(&v, 2, &out);
    int w5[] = {10, 20};
    CHECK("removing the last item works", r == 0 && out == 30 && vec_is(&v, w5, 2), "r = %d, out = %d", r, out);

    out = -1;
    r = vec_remove(&v, 2, &out);
    CHECK("removing at index len is rejected and leaves *out alone", r == -1 && out == -1 && vec_is(&v, w5, 2), "r = %d, out = %d", r, out);
    free(v.data);

    IntVec b = {NULL, 0, 0};
    for (int i = 0; i < 40; i++) vec_insert(&b, (size_t)i / 2, i);
    CHECK("40 middle inserts give len 40 and capacity 64", b.len == 40 && b.cap == 64, "len = %zu, cap = %zu", b.len, b.cap);

    size_t sorted_ok = 1;
    IntVec s = {NULL, 0, 0};
    for (int i = 0; i < 30; i++) {
        int val = (i * 7) % 30;
        size_t pos = 0;
        while (pos < s.len && s.data[pos] < val) pos++;
        if (vec_insert(&s, pos, val) != 0) sorted_ok = 0;
    }
    for (size_t i = 0; i < s.len; i++) if (s.data[i] != (int)i) sorted_ok = 0;
    CHECK("insertion sort built on vec_insert yields 0..29", sorted_ok && s.len == 30, "len = %zu", s.len);
    free(s.data);

    for (int i = 0; i < 36; i++) vec_remove(&b, 0, NULL);
    int ok2 = b.len == 4 && b.cap >= 4 && b.cap <= 16;
    CHECK("removing many items shrinks the block but keeps the rest", ok2, "len = %zu, cap = %zu", b.len, b.cap);
    CHECK("capacity stays at least 4 and shrinks to 16 or less", b.cap >= 4 && b.cap <= 16, "cap = %zu", b.cap);
    r = vec_insert(&b, 2, -1);
    CHECK("the vector still works after shrinking", r == 0 && b.len == 5 && b.data[2] == -1, "r = %d, len = %zu", r, b.len);
    while (b.len > 0) vec_remove(&b, b.len - 1, NULL);
    CHECK("emptying the vector leaves a usable block of capacity 4", b.len == 0 && b.cap == 4 && b.data != NULL, "len = %zu, cap = %zu", b.len, b.cap);
    free(b.data);
    return 0;
}
