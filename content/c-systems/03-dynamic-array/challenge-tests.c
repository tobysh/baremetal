static unsigned rng(unsigned *s) { *s = *s * 1103515245u + 12345u; return *s >> 16; }

/* A plain reference model of the same rules. */
typedef struct { int v[600]; size_t len, cap; } Model;

static void m_insert(Model *m, size_t i, int x) {
    if (m->len == m->cap) m->cap = m->cap == 0 ? 4 : m->cap * 2;
    for (size_t k = m->len; k > i; k--) m->v[k] = m->v[k - 1];
    m->v[i] = x;
    m->len++;
}
static int m_remove(Model *m, size_t i) {
    int out = m->v[i];
    for (size_t k = i; k + 1 < m->len; k++) m->v[k] = m->v[k + 1];
    m->len--;
    if (m->cap > 4 && m->len <= m->cap / 4) m->cap /= 2;
    return out;
}
static int same(const IntVec *v, const Model *m) {
    if (v->len != m->len || v->cap != m->cap) return 0;
    for (size_t i = 0; i < v->len; i++) if (v->data[i] != m->v[i]) return 0;
    return 1;
}

int main(void) {
    IntVec v;
    vec_init(&v);
    int rc = vec_insert(&v, 0, 5);
    CHECK("inserting into an empty vector at 0 works", rc == 0 && v.len == 1 && v.cap == 4 && v.data[0] == 5, "rc = %d, len = %zu, cap = %zu", rc, v.len, v.cap);
    rc = vec_insert(&v, 2, 1);
    CHECK("an index past len is rejected", rc == -1 && v.len == 1 && v.data[0] == 5, "rc = %d, len = %zu", rc, v.len);

    vec_insert(&v, 1, 7);
    vec_insert(&v, 0, 3);
    vec_insert(&v, 1, 4);
    CHECK("front, middle and end inserts shift correctly", v.len == 4 && v.data[0] == 3 && v.data[1] == 4 && v.data[2] == 5 && v.data[3] == 7, "got %d %d %d %d", v.data[0], v.data[1], v.data[2], v.data[3]);
    vec_insert(&v, 2, 100);
    CHECK("the fifth element doubles the capacity to 8", v.len == 5 && v.cap == 8 && v.data[2] == 100 && v.data[4] == 7, "len = %zu, cap = %zu", v.len, v.cap);

    int out = -1;
    rc = vec_remove_at(&v, 9, &out);
    CHECK("removing past the end fails and changes nothing", rc == -1 && out == -1 && v.len == 5, "rc = %d, out = %d, len = %zu", rc, out, v.len);
    rc = vec_remove_at(&v, 2, &out);
    CHECK("remove_at returns the element and closes the gap", rc == 0 && out == 100 && v.len == 4 && v.data[2] == 5 && v.data[3] == 7, "rc = %d, out = %d", rc, out);
    rc = vec_remove_at(&v, 0, NULL);
    CHECK("a NULL out pointer is allowed", rc == 0 && v.len == 3 && v.data[0] == 4, "rc = %d, len = %zu", rc, v.len);
    vec_free(&v);

    /* Fill to 100, then drain from various positions, comparing against the model after every single step. */
    vec_init(&v);
    Model m = { {0}, 0, 0 };
    unsigned seed = 2024u;
    int ok = 1, step_bad = -1;
    for (int step = 0; step < 100 && ok; step++) {
        size_t i = rng(&seed) % (m.len + 1);
        int x = (int)(rng(&seed) % 1000u) - 500;
        m_insert(&m, i, x);
        if (vec_insert(&v, i, x) != 0 || !same(&v, &m)) { ok = 0; step_bad = step; }
    }
    CHECK("100 inserts at random positions match the model (values and capacity)", ok, "first mismatch at insert %d (len %zu cap %zu, expected cap %zu)", step_bad, v.len, v.cap, m.cap);
    CHECK("capacity reached 128", v.cap == 128, "cap = %zu", v.cap);

    int caps_ok = 1, vals_ok = 1;
    size_t smallest_cap = v.cap;
    while (m.len > 0) {
        size_t i = rng(&seed) % m.len;
        int want = m_remove(&m, i);
        int got = -12345;
        int rc2 = vec_remove_at(&v, i, &got);
        if (rc2 != 0 || got != want) vals_ok = 0;
        if (v.len != m.len || v.cap != m.cap) caps_ok = 0;
        for (size_t k = 0; k < v.len && vals_ok; k++) if (v.data[k] != m.v[k]) vals_ok = 0;
        if (v.cap < smallest_cap) smallest_cap = v.cap;
    }
    CHECK("draining the vector returns the right values in the right order", vals_ok, "a removed value or the remaining contents were wrong");
    CHECK("capacity halves exactly when len <= cap / 4, and never below 4", caps_ok && smallest_cap == 4, "capacity differed from the rule (smallest seen %zu)", smallest_cap);
    vec_free(&v);

    vec_init(&v);
    for (int i = 0; i < 16; i++) vec_push(&v, i);
    for (int i = 0; i < 7; i++) vec_remove_at(&v, 0, NULL);
    CHECK("16 elements, 9 left: no shrink while more than a quarter is used", v.cap == 16 && v.len == 9, "cap = %zu, len = %zu", v.cap, v.len);
    for (int i = 0; i < 5; i++) vec_remove_at(&v, 0, NULL);
    CHECK("4 left of 16: shrinks to 8", v.cap == 8 && v.len == 4 && v.data[0] == 12, "cap = %zu, len = %zu", v.cap, v.len);
    vec_free(&v);
    CHECK("vec_free still resets everything", v.data == NULL && v.len == 0 && v.cap == 0, "data = %p, len = %zu, cap = %zu", (void *)v.data, v.len, v.cap);
    return 0;
}
