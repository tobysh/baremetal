static unsigned rng(unsigned *s) { *s = *s * 1103515245u + 12345u; return *s >> 16; }

int main(void) {
    StrMap m;
    CHECK("sm_init rejects 0 buckets", sm_init(&m, 0) == -1, "expected -1");
    CHECK("sm_init accepts 8 buckets", sm_init(&m, 8) == 0 && m.nbuckets == 8 && m.count == 0 && m.buckets != NULL, "failed");

    int v = -1;
    CHECK("get on an empty map fails", sm_get(&m, "x", &v) == -1 && v == -1, "expected -1");
    CHECK("del on an empty map fails", sm_del(&m, "x") == -1, "expected -1");
    CHECK("put then get", sm_put(&m, "apple", 3) == 0 && sm_get(&m, "apple", &v) == 0 && v == 3 && m.count == 1, "v %d count %zu", v, m.count);
    CHECK("updating changes the value but not the count", sm_put(&m, "apple", 4) == 0 && sm_get(&m, "apple", &v) == 0 && v == 4 && m.count == 1, "v %d count %zu", v, m.count);
    CHECK("keys are case sensitive and exact", sm_get(&m, "Apple", &v) == -1 && sm_get(&m, "appl", &v) == -1 && sm_get(&m, "apples", &v) == -1, "found a different key");
    CHECK("the empty string is a valid key", sm_put(&m, "", 9) == 0 && sm_get(&m, "", &v) == 0 && v == 9 && m.count == 2, "v %d", v);

    char buf[16];
    strcpy(buf, "mutable");
    sm_put(&m, buf, 77);
    strcpy(buf, "changed");
    CHECK("the map keeps its own copy of the key", sm_get(&m, "mutable", &v) == 0 && v == 77 && sm_get(&m, "changed", &v) == -1, "the key followed the caller's buffer");
    CHECK("deleting a key removes only that key", sm_del(&m, "apple") == 0 && sm_get(&m, "apple", &v) == -1 && sm_get(&m, "mutable", &v) == 0 && m.count == 2, "count %zu", m.count);
    CHECK("deleting twice fails the second time", sm_del(&m, "apple") == -1, "expected -1");
    sm_free(&m);
    CHECK("sm_free resets the map", m.buckets == NULL && m.nbuckets == 0 && m.count == 0, "not reset");

    StrMap one;
    sm_init(&one, 1);
    sm_put(&one, "a", 1);
    sm_put(&one, "b", 2);
    CHECK("two keys in one bucket do not trigger a resize", one.nbuckets == 1 && one.count == 2, "nbuckets %zu", one.nbuckets);
    CHECK("both keys in the same chain are found", sm_get(&one, "a", &v) == 0 && v == 1 && sm_get(&one, "b", &v) == 0 && v == 2, "v %d", v);
    CHECK("deleting the chain head keeps the rest", sm_del(&one, "b") == 0 && sm_get(&one, "a", &v) == 0 && v == 1 && sm_get(&one, "b", &v) == -1, "chain broken");
    sm_put(&one, "b", 2);
    CHECK("deleting the chain tail keeps the rest", sm_del(&one, "a") == 0 && sm_get(&one, "b", &v) == 0 && v == 2 && sm_get(&one, "a", &v) == -1, "chain broken");
    sm_put(&one, "a", 1);
    sm_put(&one, "c", 3);
    CHECK("the third key makes 3 > 2 * 1 and doubles the buckets", one.nbuckets == 2 && one.count == 3, "nbuckets %zu count %zu", one.nbuckets, one.count);
    CHECK("all keys survive the rehash", sm_get(&one, "a", &v) == 0 && v == 1 && sm_get(&one, "b", &v) == 0 && v == 2 && sm_get(&one, "c", &v) == 0 && v == 3, "a key was lost");
    sm_free(&one);

    StrMap g;
    sm_init(&g, 1);
    int ok = 1;
    char key[32];
    for (int i = 0; i < 500; i++) {
        snprintf(key, sizeof key, "key-%d", i);
        if (sm_put(&g, key, i * 7) != 0) ok = 0;
    }
    CHECK("500 inserts into a 1-bucket map succeed", ok && g.count == 500, "count %zu", g.count);
    CHECK("the table grew to keep the load factor at most 2", g.nbuckets * 2 >= g.count && g.nbuckets > 1, "nbuckets %zu for %zu entries", g.nbuckets, g.count);
    CHECK("the table did not grow absurdly", g.nbuckets <= 1024, "nbuckets %zu", g.nbuckets);
    ok = 1;
    for (int i = 0; i < 500; i++) {
        snprintf(key, sizeof key, "key-%d", i);
        if (sm_get(&g, key, &v) != 0 || v != i * 7) ok = 0;
    }
    CHECK("all 500 values are retrievable after growing", ok, "a value was lost or wrong");
    sm_free(&g);

    StrMap r;
    sm_init(&r, 4);
    int has[40], val[40];
    for (int i = 0; i < 40; i++) has[i] = 0;
    size_t live = 0;
    unsigned seed = 1234567u;
    int step = 0;
    ok = 1;
    for (; step < 3000 && ok; step++) {
        int idx = (int)(rng(&seed) % 40u);
        snprintf(key, sizeof key, "w%d_%d", idx, idx * idx);
        unsigned op = rng(&seed) % 3u;
        if (op == 0) {
            int nv = (int)(rng(&seed) % 1000u);
            if (sm_put(&r, key, nv) != 0) ok = 0;
            if (!has[idx]) live++;
            has[idx] = 1; val[idx] = nv;
        } else if (op == 1) {
            int rc = sm_del(&r, key);
            if (has[idx]) { if (rc != 0) ok = 0; has[idx] = 0; live--; } else if (rc != -1) ok = 0;
        } else {
            int got = -1;
            int rc = sm_get(&r, key, &got);
            if (has[idx]) { if (rc != 0 || got != val[idx]) ok = 0; } else if (rc != -1) ok = 0;
        }
        if (r.count != live) ok = 0;
    }
    CHECK("3000 random put/del/get steps match a reference", ok, "diverged at step %d", step);
    sm_free(&r);
    return 0;
}
