static unsigned rng(unsigned *s) { *s = *s * 1103515245u + 12345u; return *s >> 16; }

int main(void) {
    CHECK("lru_new(0) is NULL", lru_new(0) == NULL, "expected NULL");
    Lru *c = lru_new(2);
    CHECK("lru_new(2) returns a cache", c != NULL, "got NULL");
    if (c == NULL) return 0;
    int v = -1, k = -1;
    CHECK("a new cache is empty", lru_size(c) == 0 && lru_oldest(c, &k) == -1 && k == -1, "size %zu", lru_size(c));
    CHECK("get on an empty cache misses", lru_get(c, 1, &v) == -1 && v == -1, "expected a miss");

    lru_put(c, 1, 10);
    lru_put(c, 2, 20);
    CHECK("two puts give size 2", lru_size(c) == 2, "size %zu", lru_size(c));
    CHECK("the oldest is the first key", lru_oldest(c, &k) == 0 && k == 1, "oldest %d", k);
    CHECK("get hits and returns the value", lru_get(c, 1, &v) == 0 && v == 10, "v %d", v);
    CHECK("a hit makes that key the newest, so 2 is oldest", lru_oldest(c, &k) == 0 && k == 2, "oldest %d", k);
    lru_put(c, 3, 30);
    CHECK("the third put evicts the least recently used (2)", lru_size(c) == 2 && lru_get(c, 2, &v) == -1, "key 2 is still there");
    CHECK("keys 1 and 3 survive", lru_get(c, 1, &v) == 0 && v == 10 && lru_get(c, 3, &v) == 0 && v == 30, "v %d", v);
    CHECK("after those gets, 1 is the oldest", lru_oldest(c, &k) == 0 && k == 1, "oldest %d", k);
    lru_put(c, 1, 11);
    CHECK("updating a key changes its value and refreshes it", lru_size(c) == 2 && lru_oldest(c, &k) == 0 && k == 3 && lru_get(c, 1, &v) == 0 && v == 11, "v %d oldest %d", v, k);
    lru_put(c, 4, 40);
    CHECK("evicts 3 next", lru_get(c, 3, &v) == -1 && lru_get(c, 4, &v) == 0 && v == 40 && lru_get(c, 1, &v) == 0, "wrong eviction");
    lru_free(c);

    Lru *one = lru_new(1);
    lru_put(one, 7, 70);
    lru_put(one, 8, 80);
    CHECK("capacity 1 keeps only the newest", lru_size(one) == 1 && lru_get(one, 7, &v) == -1 && lru_get(one, 8, &v) == 0 && v == 80, "wrong");
    lru_put(one, 8, 81);
    CHECK("capacity 1 update keeps one entry", lru_size(one) == 1 && lru_get(one, 8, &v) == 0 && v == 81 && lru_oldest(one, &k) == 0 && k == 8, "wrong");
    lru_free(one);

    Lru *neg = lru_new(3);
    lru_put(neg, -1, 1);
    lru_put(neg, 0, 2);
    lru_put(neg, -2147483647 - 1, 3);
    lru_put(neg, 2147483647, 4);
    CHECK("negative and extreme keys work (and -1 is evicted first)", lru_get(neg, -1, &v) == -1 && lru_get(neg, 0, &v) == 0 && v == 2 && lru_get(neg, -2147483647 - 1, &v) == 0 && v == 3 && lru_get(neg, 2147483647, &v) == 0 && v == 4, "v %d", v);
    lru_free(neg);

    Lru *r = lru_new(7);
    int mk[7], mv[7], mlen = 0;   /* model: index 0 = most recent */
    unsigned seed = 424242u;
    int ok = 1, step = 0;
    for (; step < 5000 && ok; step++) {
        int key = (int)(rng(&seed) % 20u) - 5;
        int idx = -1;
        for (int i = 0; i < mlen; i++) if (mk[i] == key) idx = i;
        if (rng(&seed) % 2u) {
            int nv = (int)(rng(&seed) % 1000u);
            lru_put(r, key, nv);
            if (idx >= 0) { memmove(mk + 1, mk, (size_t)idx * sizeof(int)); memmove(mv + 1, mv, (size_t)idx * sizeof(int)); }
            else {
                if (mlen == 7) mlen--;
                memmove(mk + 1, mk, (size_t)mlen * sizeof(int)); memmove(mv + 1, mv, (size_t)mlen * sizeof(int));
                mlen++;
            }
            mk[0] = key; mv[0] = nv;
        } else {
            int got = -1;
            int rc = lru_get(r, key, &got);
            if (idx < 0) { if (rc != -1) ok = 0; }
            else {
                int val = mv[idx];
                if (rc != 0 || got != val) ok = 0;
                memmove(mk + 1, mk, (size_t)idx * sizeof(int)); memmove(mv + 1, mv, (size_t)idx * sizeof(int));
                mk[0] = key; mv[0] = val;
            }
        }
        int ok_key = -99;
        if (lru_size(r) != (size_t)mlen) ok = 0;
        if (mlen > 0 && (lru_oldest(r, &ok_key) != 0 || ok_key != mk[mlen - 1])) ok = 0;
    }
    CHECK("5000 random puts and gets match a reference LRU model", ok, "diverged at step %d", step);
    lru_free(r);
    lru_free(NULL);

    Lru *big = lru_new(1000);
    ok = 1;
    for (int i = 0; i < 30000; i++) {
        lru_put(big, i, i * 2);
        int got;
        if (i >= 1000 && (lru_get(big, i - 1, &got) != 0 || got != (i - 1) * 2)) ok = 0;
    }
    int got2;
    CHECK("a cache of 1000 over 30000 puts keeps exactly the recent keys", ok && lru_size(big) == 1000 && lru_get(big, 29999, &got2) == 0 && got2 == 59998 && lru_get(big, 5, &got2) == -1, "size %zu", lru_size(big));
    lru_free(big);
    return 0;
}
