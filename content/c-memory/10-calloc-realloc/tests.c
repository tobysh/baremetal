static int vec_has_squares(const IntVec *v, int count) {
    if (v->len != (size_t)count || v->data == NULL) return 0;
    for (int i = 0; i < count; i++) if (v->data[i] != i * i) return 0;
    return 1;
}

int main(void) {
    int *z = new_zeroed(50);
    int allz = z != NULL;
    for (int i = 0; allz && i < 50; i++) if (z[i] != 0) allz = 0;
    CHECK("new_zeroed(50) returns 50 zeroed ints", allz, "NULL or non-zero element");
    if (z) {
        z[49] = 7;
        CHECK("the last element of new_zeroed(50) is writable", z[49] == 7, "write failed");
    }
    free(z);

    int *z1 = new_zeroed(1);
    CHECK("new_zeroed(1) is {0}", z1 != NULL && z1[0] == 0, "bad block");
    free(z1);

    IntVec v = {NULL, 0, 0};
    int r = vec_push(&v, 0);
    CHECK("the first push succeeds and gives capacity 4", r == 0 && v.len == 1 && v.cap == 4 && v.data != NULL && v.data[0] == 0, "r = %d, len = %zu, cap = %zu", r, v.len, v.cap);

    for (int i = 1; i < 4; i++) vec_push(&v, i * i);
    CHECK("four pushes still fit in capacity 4", v.len == 4 && v.cap == 4 && vec_has_squares(&v, 4), "len = %zu, cap = %zu", v.len, v.cap);

    vec_push(&v, 16);
    CHECK("the fifth push doubles capacity to 8 and keeps old values", v.len == 5 && v.cap == 8 && vec_has_squares(&v, 5), "len = %zu, cap = %zu", v.len, v.cap);

    for (int i = 5; i < 100; i++) vec_push(&v, i * i);
    CHECK("100 pushes keep every value in order", vec_has_squares(&v, 100), "len = %zu", v.len);
    CHECK("capacity after 100 pushes is 128 (4 doubled up)", v.cap == 128, "cap = %zu", v.cap);

    r = vec_shrink(&v);
    CHECK("vec_shrink makes capacity equal to length and keeps contents", r == 0 && v.cap == 100 && vec_has_squares(&v, 100), "r = %d, cap = %zu", r, v.cap);
    r = vec_push(&v, 100 * 100);
    CHECK("pushing after a shrink grows again and works", r == 0 && vec_has_squares(&v, 101) && v.cap >= 101, "r = %d, len = %zu, cap = %zu", r, v.len, v.cap);
    free(v.data);

    IntVec w = {NULL, 0, 0};
    for (int i = 0; i < 9; i++) vec_push(&w, i * i);
    CHECK("nine pushes give capacity 16", w.cap == 16 && vec_has_squares(&w, 9), "cap = %zu", w.cap);
    vec_shrink(&w);
    CHECK("shrinking a 9-of-16 vector gives capacity 9", w.cap == 9 && vec_has_squares(&w, 9), "cap = %zu", w.cap);
    free(w.data);

    IntVec e = {NULL, 0, 0};
    r = vec_shrink(&e);
    CHECK("shrinking an empty vector is fine and leaves it empty", r == 0 && e.data == NULL && e.len == 0 && e.cap == 0, "r = %d", r);

    IntVec f = {NULL, 0, 0};
    vec_push(&f, 5);
    f.len = 0;
    r = vec_shrink(&f);
    CHECK("shrinking to length 0 frees the block and resets data and cap", r == 0 && f.data == NULL && f.cap == 0, "data = %p, cap = %zu", (void *)f.data, f.cap);
    return 0;
}
