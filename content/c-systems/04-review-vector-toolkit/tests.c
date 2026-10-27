static int twice(int x) { return x * 2; }
static int negate(int x) { return -x; }
static int is_even(int x) { return x % 2 == 0; }
static int is_positive(int x) { return x > 0; }
static int always(int x) { (void)x; return 1; }
static int never(int x) { (void)x; return 0; }
static int add(int acc, int x) { return acc + x; }
static int max_of(int acc, int x) { return x > acc ? x : acc; }
static int times(int acc, int x) { return acc * x; }

static IntVec make(const int *src, size_t n) {
    IntVec v;
    vec_init(&v);
    for (size_t i = 0; i < n; i++) vec_push(&v, src[i]);
    return v;
}

int main(void) {
    int src[] = {3, -1, 4, -1, 5, -9, 2, 6};
    IntVec v = make(src, 8);

    vec_map(&v, twice);
    CHECK("vec_map doubles every element", v.data[0] == 6 && v.data[1] == -2 && v.data[7] == 12, "got %d, %d, ..., %d", v.data[0], v.data[1], v.data[7]);
    CHECK("vec_map does not change len", v.len == 8, "len is %zu", v.len);
    vec_map(&v, negate);
    CHECK("vec_map works with a different callback", v.data[0] == -6 && v.data[5] == 18, "got %d and %d", v.data[0], v.data[5]);
    vec_free(&v);

    IntVec e;
    vec_init(&e);
    vec_map(&e, twice);
    CHECK("vec_map on an empty vector is safe", e.len == 0 && e.data == NULL, "len %zu", e.len);
    CHECK("vec_filter on an empty vector removes 0", vec_filter(&e, always) == 0, "expected 0");
    CHECK("vec_reduce on an empty vector returns init", vec_reduce(&e, 77, add) == 77, "expected 77");

    IntVec f = make(src, 8);
    size_t cap_before = f.cap;
    int *data_before = f.data;
    size_t removed = vec_filter(&f, is_positive);
    CHECK("vec_filter returns how many were removed", removed == 3, "got %zu", removed);
    CHECK("vec_filter updates len", f.len == 5, "len is %zu", f.len);
    CHECK("vec_filter keeps the original order", f.data[0] == 3 && f.data[1] == 4 && f.data[2] == 5 && f.data[3] == 2 && f.data[4] == 6,
          "got %d %d %d %d %d", f.data[0], f.data[1], f.data[2], f.data[3], f.data[4]);
    CHECK("vec_filter does not reallocate", f.cap == cap_before && f.data == data_before, "cap or data changed");
    CHECK("vec_filter with a different predicate", vec_filter(&f, is_even) == 2 && f.len == 3 && f.data[0] == 4 && f.data[1] == 2 && f.data[2] == 6, "len is %zu", f.len);
    CHECK("vec_filter(always) removes nothing", vec_filter(&f, always) == 0 && f.len == 3, "len is %zu", f.len);
    CHECK("vec_filter(never) removes everything", vec_filter(&f, never) == 3 && f.len == 0, "len is %zu", f.len);
    vec_free(&f);

    IntVec r = make(src, 8);
    CHECK("vec_reduce sums", vec_reduce(&r, 0, add) == 9, "got %d", vec_reduce(&r, 0, add));
    CHECK("vec_reduce starts from init", vec_reduce(&r, 100, add) == 109, "got %d", vec_reduce(&r, 100, add));
    CHECK("vec_reduce finds a maximum", vec_reduce(&r, -1000, max_of) == 6, "got %d", vec_reduce(&r, -1000, max_of));
    int small[] = {2, 3, 4};
    IntVec s = make(small, 3);
    CHECK("vec_reduce multiplies", vec_reduce(&s, 1, times) == 24, "got %d", vec_reduce(&s, 1, times));
    CHECK("vec_reduce leaves the vector unchanged", r.len == 8 && r.data[0] == 3 && r.data[7] == 6, "vector was modified");
    vec_free(&s);
    vec_free(&r);
    return 0;
}
