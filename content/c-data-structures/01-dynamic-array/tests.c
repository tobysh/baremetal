static unsigned rng(unsigned *s) { *s = *s * 1103515245u + 12345u; return *s >> 16; }

int main(void) {
    IntVec v;
    v.data = (int *)0x1; v.len = 99; v.cap = 99;
    vec_init(&v);
    CHECK("vec_init makes an empty vector", v.data == NULL && v.len == 0 && v.cap == 0, "data %p, len %zu, cap %zu", (void *)v.data, v.len, v.cap);

    int out = -7;
    CHECK("vec_get on an empty vector fails", vec_get(&v, 0, &out) == -1 && out == -7, "expected -1 and *out untouched");
    CHECK("vec_pop on an empty vector fails", vec_pop(&v, &out) == -1 && out == -7, "expected -1 and *out untouched");

    CHECK("first push succeeds", vec_push(&v, 42) == 0 && v.len == 1 && v.data != NULL, "len %zu", v.len);
    CHECK("first push gives capacity 4", v.cap == 4, "cap is %zu", v.cap);
    CHECK("vec_get returns the pushed value", vec_get(&v, 0, &out) == 0 && out == 42, "out %d", out);
    CHECK("vec_get past the end fails", vec_get(&v, 1, &out) == -1 && out == 42, "expected -1");

    for (int i = 1; i < 5; i++) vec_push(&v, 42 + i);
    CHECK("the fifth push doubles capacity to 8", v.len == 5 && v.cap == 8, "len %zu cap %zu", v.len, v.cap);

    IntVec w;
    vec_init(&w);
    int changes = 0;
    size_t last_cap = 0;
    int ok = 1;
    for (int i = 0; i < 1000; i++) {
        if (vec_push(&w, i * 3 - 100) != 0) { ok = 0; break; }
        if (w.cap != last_cap) { changes++; last_cap = w.cap; }
        if (w.cap < w.len) ok = 0;
    }
    CHECK("1000 pushes keep len <= cap", ok && w.len == 1000, "len %zu cap %zu", w.len, w.cap);
    CHECK("capacity grows geometrically (few reallocations)", changes <= 12, "capacity changed %d times in 1000 pushes", changes);
    CHECK("capacity is not wildly larger than needed", w.cap <= 2048, "cap %zu", w.cap);
    ok = 1;
    for (int i = 0; i < 1000; i++) {
        int got;
        if (vec_get(&w, (size_t)i, &got) != 0 || got != i * 3 - 100) { ok = 0; break; }
    }
    CHECK("all 1000 values survive the reallocations", ok, "a value changed after growing");

    int top = 0;
    CHECK("vec_pop returns the last value", vec_pop(&w, &top) == 0 && top == 999 * 3 - 100 && w.len == 999, "got %d, len %zu", top, w.len);
    size_t capb = w.cap;
    for (int i = 0; i < 500; i++) vec_pop(&w, &top);
    CHECK("popping does not shrink capacity", w.len == 499 && w.cap == capb, "len %zu cap %zu", w.len, w.cap);
    vec_free(&w);
    CHECK("vec_free resets the vector", w.data == NULL && w.len == 0 && w.cap == 0, "not reset");
    vec_free(&w);
    CHECK("vec_free twice is harmless", w.data == NULL, "second free changed state");

    IntVec u;
    vec_init(&u);
    int first[] = {10, 20, 30};
    for (int i = 0; i < 3; i++) vec_push(&u, first[i]);
    CHECK("insert in the middle succeeds", vec_insert(&u, 1, 15) == 0, "returned non-zero");
    CHECK("insert at the front succeeds", vec_insert(&u, 0, 5) == 0, "returned non-zero");
    CHECK("insert at len appends", vec_insert(&u, u.len, 40) == 0, "returned non-zero");
    int expect[] = {5, 10, 15, 20, 30, 40};
    ok = u.len == 6;
    for (size_t i = 0; ok && i < 6; i++) if (u.data[i] != expect[i]) ok = 0;
    CHECK("inserts leave 5 10 15 20 30 40", ok, "len %zu: %d %d %d %d %d %d", u.len, u.data[0], u.data[1], u.data[2], u.data[3], u.data[4], u.data[5]);
    CHECK("insert past len fails and changes nothing", vec_insert(&u, 8, 99) == -1 && u.len == 6 && u.data[5] == 40, "len %zu", u.len);

    unsigned seed = 2024u;
    int model[300];
    int mlen = 0;
    IntVec r;
    vec_init(&r);
    ok = 1;
    for (int step = 0; step < 300 && ok; step++) {
        int pos = (int)(rng(&seed) % (unsigned)(mlen + 1));
        int val = (int)(rng(&seed) % 1000u);
        if (vec_insert(&r, (size_t)pos, val) != 0) { ok = 0; break; }
        memmove(&model[pos + 1], &model[pos], (size_t)(mlen - pos) * sizeof(int));
        model[pos] = val;
        mlen++;
    }
    for (int i = 0; ok && i < mlen; i++) if (r.data[i] != model[i]) ok = 0;
    CHECK("300 random inserts match a reference", ok && (int)r.len == mlen, "mismatch");
    vec_free(&r);
    vec_free(&u);
    vec_free(&v);
    return 0;
}
