int main(void) {
    IntVec v;
    vec_init(&v);
    CHECK("vec_init makes an empty vector", v.data == NULL && v.len == 0 && v.cap == 0, "data = %p, len = %zu, cap = %zu", (void *)v.data, v.len, v.cap);

    int rc = vec_push(&v, 11);
    CHECK("the first push succeeds and stores the value", rc == 0 && v.len == 1 && v.data != NULL && v.data[0] == 11, "rc = %d, len = %zu", rc, v.len);
    CHECK("capacity is at least the length", v.cap >= v.len, "cap = %zu, len = %zu", v.cap, v.len);

    vec_push(&v, 22);
    vec_push(&v, 33);
    CHECK("pushes keep their order", v.len == 3 && v.data[0] == 11 && v.data[1] == 22 && v.data[2] == 33, "len = %zu", v.len);

    vec_free(&v);
    vec_init(&v);
    int ok = 1;
    for (int i = 0; i < 1000; i++) {
        if (vec_push(&v, i * 3) != 0) ok = 0;
    }
    CHECK("1000 pushes all succeed", ok && v.len == 1000, "len = %zu", v.len);
    ok = v.data != NULL;
    for (int i = 0; ok && i < 1000; i++) if (v.data[i] != i * 3) ok = 0;
    CHECK("every value survived the growing", ok, "some value was lost or corrupted");
    CHECK("capacity covers the length", v.cap >= 1000, "cap = %zu", v.cap);
    CHECK("capacity grows geometrically, not one slot at a time (cap <= 2 * len)", v.cap <= 2000, "cap = %zu", v.cap);

    int out = -1;
    rc = vec_pop(&v, &out);
    CHECK("vec_pop returns the last element", rc == 0 && out == 999 * 3, "rc = %d, out = %d", rc, out);
    CHECK("vec_pop shrinks the length by one", v.len == 999, "len = %zu", v.len);

    vec_push(&v, 12345);
    CHECK("pushing after a pop reuses the slot", v.len == 1000 && v.data[999] == 12345, "len = %zu", v.len);

    vec_free(&v);
    CHECK("vec_free resets the vector", v.data == NULL && v.len == 0 && v.cap == 0, "data = %p, len = %zu, cap = %zu", (void *)v.data, v.len, v.cap);
    vec_free(&v);
    CHECK("vec_free twice is harmless", v.data == NULL && v.len == 0, "state changed");

    IntVec e;
    vec_init(&e);
    int keep = 77;
    rc = vec_pop(&e, &keep);
    CHECK("popping an empty vector returns -1 and leaves *out alone", rc == -1 && keep == 77, "rc = %d, keep = %d", rc, keep);
    vec_free(&e);
    return 0;
}
