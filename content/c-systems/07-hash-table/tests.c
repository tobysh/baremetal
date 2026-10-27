static int count_entries(const HashTable *t) {
    int n = 0;
    for (int i = 0; i < BUCKETS; i++) {
        for (const Entry *e = t->buckets[i]; e != NULL; e = e->next) n++;
    }
    return n;
}

int main(void) {
    HashTable t;
    ht_init(&t);
    int empty = 1;
    for (int i = 0; i < BUCKETS; i++) if (t.buckets[i] != NULL) empty = 0;
    CHECK("ht_init leaves every bucket NULL", empty, "a bucket was not NULL");

    int v = -1;
    CHECK("looking up a key in an empty table finds nothing", ht_get(&t, "alice", &v) == 0 && v == -1, "v = %d", v);

    CHECK("ht_put succeeds", ht_put(&t, "alice", 31) == 0, "ht_put failed");
    CHECK("ht_get finds the stored value", ht_get(&t, "alice", &v) == 1 && v == 31, "v = %d", v);

    ht_put(&t, "bob", 45);
    ht_put(&t, "carol", 0);
    ht_put(&t, "dave", -8);
    CHECK("several keys can live together", ht_get(&t, "bob", &v) == 1 && v == 45 && ht_get(&t, "alice", &v) == 1 && v == 31, "lookup failed");
    CHECK("zero is a real, findable value", ht_get(&t, "carol", &v) == 1 && v == 0, "v = %d", v);
    CHECK("negative values are fine", ht_get(&t, "dave", &v) == 1 && v == -8, "v = %d", v);

    v = 123;
    CHECK("a missing key returns 0 and leaves *out alone", ht_get(&t, "eve", &v) == 0 && v == 123, "v = %d", v);
    CHECK("keys are case sensitive", ht_get(&t, "Alice", &v) == 0, "found Alice");

    ht_put(&t, "alice", 32);
    CHECK("putting an existing key updates the value", ht_get(&t, "alice", &v) == 1 && v == 32, "v = %d", v);
    CHECK("updating does not add a duplicate entry", count_entries(&t) == 4, "entries = %d", count_entries(&t));

    char buf[16];
    strcpy(buf, "temp");
    ht_put(&t, buf, 7);
    strcpy(buf, "XXXX");
    CHECK("the table keeps its own copy of the key", ht_get(&t, "temp", &v) == 1 && v == 7 && ht_get(&t, "XXXX", &v) == 0, "key was not copied");

    ht_free(&t);
    empty = 1;
    for (int i = 0; i < BUCKETS; i++) if (t.buckets[i] != NULL) empty = 0;
    CHECK("ht_free leaves every bucket NULL", empty, "a bucket was not NULL");

    ht_init(&t);
    int ok = 1;
    for (int i = 0; i < 500; i++) {
        char key[24];
        sprintf(key, "villager-%d", i);
        if (ht_put(&t, key, i * 2) != 0) ok = 0;
    }
    CHECK("500 keys go in (far more than 16 buckets, so chains must work)", ok && count_entries(&t) == 500, "entries = %d", count_entries(&t));
    ok = 1;
    for (int i = 0; i < 500; i++) {
        char key[24];
        sprintf(key, "villager-%d", i);
        if (ht_get(&t, key, &v) != 1 || v != i * 2) ok = 0;
    }
    CHECK("all 500 keys are found with the right values", ok, "some lookup failed");
    CHECK("a key that was never added is still missing", ht_get(&t, "villager-500", &v) == 0, "found a key that was never added");
    ht_free(&t);
    return 0;
}
