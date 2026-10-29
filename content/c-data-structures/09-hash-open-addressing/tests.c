static unsigned rng(unsigned *s) { *s = *s * 1103515245u + 12345u; return *s >> 16; }
static size_t home(int key, size_t cap) { return (size_t)((unsigned)key * 2654435761u) & (cap - 1); }

int main(void) {
    OAMap m;
    CHECK("oa_init rejects capacity 0, 1 and 6", oa_init(&m, 0) == -1 && oa_init(&m, 1) == -1 && oa_init(&m, 6) == -1, "expected -1");
    CHECK("oa_init accepts 8", oa_init(&m, 8) == 0 && m.cap == 8 && m.count == 0 && m.slots != NULL, "failed");
    int ok = 1;
    for (size_t i = 0; i < 8; i++) if (m.slots[i].state != EMPTY) ok = 0;
    CHECK("all slots start EMPTY", ok, "a slot is not EMPTY");

    int v = -1;
    CHECK("get on an empty table fails", oa_get(&m, 5, &v) == -1 && v == -1, "expected -1");
    CHECK("del on an empty table fails", oa_del(&m, 5) == -1, "expected -1");
    CHECK("put and get one pair", oa_put(&m, 5, 50) == 0 && oa_get(&m, 5, &v) == 0 && v == 50 && m.count == 1, "v %d count %zu", v, m.count);
    CHECK("updating a key changes the value, not the count", oa_put(&m, 5, 51) == 0 && oa_get(&m, 5, &v) == 0 && v == 51 && m.count == 1, "v %d count %zu", v, m.count);
    CHECK("negative and zero keys work", oa_put(&m, 0, 1) == 0 && oa_put(&m, -7, 2) == 0 && oa_get(&m, 0, &v) == 0 && v == 1 && oa_get(&m, -7, &v) == 0 && v == 2, "v %d", v);

    /* Find a pair of colliding keys for a 16-slot table. */
    OAMap c;
    oa_init(&c, 16);
    int k1 = 1, k2 = -1, k3 = -1;
    for (int k = 2; k < 10000 && k3 < 0; k++) {
        if (home(k, 16) == home(k1, 16)) { if (k2 < 0) k2 = k; else k3 = k; }
    }
    oa_put(&c, k1, 10);
    oa_put(&c, k2, 20);
    oa_put(&c, k3, 30);
    CHECK("three colliding keys all retrievable", oa_get(&c, k1, &v) == 0 && v == 10 && oa_get(&c, k2, &v) == 0 && v == 20 && oa_get(&c, k3, &v) == 0 && v == 30 && c.count == 3, "v %d", v);
    CHECK("deleting the first of a collision chain succeeds", oa_del(&c, k1) == 0 && c.count == 2, "count %zu", c.count);
    CHECK("keys further along the chain are still found (tombstone)", oa_get(&c, k2, &v) == 0 && v == 20 && oa_get(&c, k3, &v) == 0 && v == 30, "v %d: a lookup stopped at the deleted slot", v);
    CHECK("the deleted key is gone", oa_get(&c, k1, &v) == -1 && oa_del(&c, k1) == -1, "still found");
    CHECK("re-adding a deleted key works", oa_put(&c, k1, 11) == 0 && oa_get(&c, k1, &v) == 0 && v == 11 && c.count == 3, "v %d", v);
    oa_del(&c, k2);
    oa_put(&c, k3, 33);
    ok = 1;
    int seen3 = 0;
    for (size_t i = 0; i < 16; i++) if (c.slots[i].state == USED && c.slots[i].key == k3) seen3++;
    CHECK("updating a key past a tombstone does not duplicate it", seen3 == 1 && oa_get(&c, k3, &v) == 0 && v == 33 && c.count == 2, "found %d copies, count %zu", seen3, c.count);
    oa_free(&c);
    CHECK("oa_free resets the map", c.slots == NULL && c.cap == 0 && c.count == 0, "not reset");

    OAMap f;
    oa_init(&f, 8);
    ok = 1;
    for (int k = 0; k < 8; k++) if (oa_put(&f, k * 13 + 1, k) != 0) ok = 0;
    CHECK("a table of 8 slots takes 8 distinct keys", ok && f.count == 8, "count %zu", f.count);
    CHECK("a 9th new key is refused", oa_put(&f, 9999, 1) == -1 && f.count == 8, "expected -1");
    CHECK("updating an existing key still works when full", oa_put(&f, 1, 77) == 0 && oa_get(&f, 1, &v) == 0 && v == 77, "v %d", v);
    CHECK("looking up an absent key in a full table terminates", oa_get(&f, 424242, &v) == -1, "expected -1");
    CHECK("deleting an absent key in a full table terminates", oa_del(&f, 424242) == -1, "expected -1");
    oa_del(&f, 14);
    oa_del(&f, 27);
    oa_del(&f, 40);
    CHECK("freed slots are reusable", oa_put(&f, 500, 1) == 0 && oa_put(&f, 501, 2) == 0 && oa_put(&f, 502, 3) == 0 && f.count == 8, "count %zu", f.count);
    CHECK("a full table of live keys refuses again", oa_put(&f, 503, 4) == -1, "expected -1");
    CHECK("new keys stored in reused slots are found", oa_get(&f, 500, &v) == 0 && v == 1 && oa_get(&f, 502, &v) == 0 && v == 3 && oa_get(&f, 1, &v) == 0 && v == 77, "v %d", v);
    oa_free(&f);

    OAMap r;
    oa_init(&r, 64);
    int has[50], val[50];
    for (int i = 0; i < 50; i++) has[i] = 0;
    size_t live = 0;
    unsigned seed = 8675309u;
    int step = 0;
    ok = 1;
    for (; step < 3000 && ok; step++) {
        int key = (int)(rng(&seed) % 50u) - 10;
        int idx = key + 10;
        unsigned op = rng(&seed) % 3u;
        if (op == 0) {
            int nv = (int)(rng(&seed) % 1000u);
            if (oa_put(&r, key, nv) != 0) ok = 0;
            if (!has[idx]) live++;
            has[idx] = 1; val[idx] = nv;
        } else if (op == 1) {
            int rc = oa_del(&r, key);
            if (has[idx]) { if (rc != 0) ok = 0; has[idx] = 0; live--; } else if (rc != -1) ok = 0;
        } else {
            int got = -1;
            int rc = oa_get(&r, key, &got);
            if (has[idx]) { if (rc != 0 || got != val[idx]) ok = 0; } else if (rc != -1) ok = 0;
        }
        if (r.count != live) ok = 0;
    }
    CHECK("3000 random put/del/get steps match a reference", ok, "diverged at step %d", step);
    oa_free(&r);
    oa_free(&m);
    return 0;
}
