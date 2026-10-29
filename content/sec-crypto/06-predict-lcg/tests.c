static uint32_t st;
static uint16_t gen(void) { st = st * 1664525u + 1013904223u; return (uint16_t)(st >> 16); }

int main(void) {
    uint32_t seeds[] = {1, 42, 20240101u, 0xdeadbeefu, 7, 99999, 123456789u};
    for (int i = 0; i < 7; i++) {
        st = seeds[i];
        uint16_t o1 = gen(), o2 = gen(), o3 = gen(), o4 = gen();
        uint16_t p = predict_next(o1, o2, o3);
        char name[64];
        snprintf(name, sizeof name, "predicts the 4th output for seed %u", seeds[i]);
        CHECK(name, p == o4, "saw %u %u %u, expected %u, got %u", o1, o2, o3, o4, p);
    }
    st = 31337;
    for (int i = 0; i < 20; i++) gen();
    uint16_t a = gen(), b = gen(), c = gen(), d = gen();
    CHECK("works in the middle of a long run", predict_next(a, b, c) == d, "expected %u", d);
    uint16_t e = gen(), f = gen(), g = gen(), h = gen();
    CHECK("works again on the next window of outputs", predict_next(e, f, g) == h, "expected %u", h);
    return 0;
}
