static unsigned rng(unsigned *s) { *s = *s * 1103515245u + 12345u; return *s >> 16; }
static int is_heap(const Heap *h) {
    for (size_t i = 1; i < h->len; i++) if (h->a[(i - 1) / 2] > h->a[i]) return 0;
    return 1;
}

int main(void) {
    Heap h;
    CHECK("heap_init rejects capacity 0", heap_init(&h, 0) == -1 && h.a == NULL, "expected -1 and NULL");
    CHECK("heap_init accepts capacity 16", heap_init(&h, 16) == 0 && h.a != NULL && h.len == 0 && h.cap == 16, "failed");
    int out = -5;
    CHECK("pop on an empty heap fails", heap_pop(&h, &out) == -1 && out == -5, "expected -1, *out untouched");
    CHECK("peek on an empty heap fails", heap_peek(&h, &out) == -1 && out == -5, "expected -1, *out untouched");

    CHECK("push 5 into an empty heap", heap_push(&h, 5) == 0 && heap_peek(&h, &out) == 0 && out == 5, "out %d", out);
    heap_push(&h, 3);
    CHECK("a smaller push becomes the minimum", heap_peek(&h, &out) == 0 && out == 3 && h.a[0] == 3 && h.a[1] == 5, "a[0] %d a[1] %d", h.a[0], h.a[1]);
    heap_push(&h, 8);
    heap_push(&h, 1);
    heap_push(&h, 9);
    heap_push(&h, 2);
    CHECK("the heap property holds after six pushes", is_heap(&h) && h.len == 6, "heap order broken: %d %d %d %d %d %d", h.a[0], h.a[1], h.a[2], h.a[3], h.a[4], h.a[5]);
    CHECK("peek does not remove", heap_peek(&h, &out) == 0 && out == 1 && h.len == 6, "out %d len %zu", out, h.len);

    int expect[] = {1, 2, 3, 5, 8, 9};
    int ok = 1;
    for (int i = 0; i < 6; i++) {
        if (heap_pop(&h, &out) != 0 || out != expect[i] || !is_heap(&h)) ok = 0;
    }
    CHECK("pops come out 1 2 3 5 8 9, with the property kept", ok && h.len == 0, "wrong order or broken heap");
    CHECK("empty again after popping everything", heap_pop(&h, &out) == -1, "expected -1");
    heap_free(&h);
    CHECK("heap_free resets the heap", h.a == NULL && h.len == 0 && h.cap == 0, "not reset");

    Heap f;
    heap_init(&f, 3);
    CHECK("fill a capacity-3 heap", heap_push(&f, 7) == 0 && heap_push(&f, 7) == 0 && heap_push(&f, 6) == 0, "failed");
    CHECK("pushing into a full heap fails", heap_push(&f, 1) == -1 && f.len == 3, "expected -1");
    ok = heap_pop(&f, &out) == 0 && out == 6 && heap_pop(&f, &out) == 0 && out == 7 && heap_pop(&f, &out) == 0 && out == 7;
    CHECK("duplicates are kept", ok, "last out %d", out);
    heap_free(&f);

    Heap neg;
    heap_init(&neg, 4);
    heap_push(&neg, 0);
    heap_push(&neg, -2147483647 - 1);
    heap_push(&neg, 2147483647);
    heap_push(&neg, -1);
    heap_pop(&neg, &out);
    CHECK("negative and extreme values", out == -2147483647 - 1 && is_heap(&neg), "popped %d", out);
    heap_free(&neg);

    Heap r;
    heap_init(&r, 40);
    int model[40], mlen = 0;
    unsigned seed = 99991u;
    int step = 0;
    ok = 1;
    for (; step < 4000 && ok; step++) {
        if (rng(&seed) % 5u < 3u) {
            int x = (int)(rng(&seed) % 100u) - 50;
            int rc = heap_push(&r, x);
            if (mlen == 40) { if (rc != -1) ok = 0; }
            else { if (rc != 0) ok = 0; model[mlen++] = x; }
        } else {
            int got = 12345;
            int rc = heap_pop(&r, &got);
            if (mlen == 0) { if (rc != -1) ok = 0; }
            else {
                int mi = 0;
                for (int i = 1; i < mlen; i++) if (model[i] < model[mi]) mi = i;
                if (rc != 0 || got != model[mi]) ok = 0;
                model[mi] = model[--mlen];
            }
        }
        if (!is_heap(&r) || (int)r.len != mlen) ok = 0;
    }
    CHECK("4000 random pushes and pops match a reference minimum", ok, "diverged at step %d", step);
    heap_free(&r);

    int s1[] = {5, 2, 9, 1, 5, 6, 0, -3, 7};
    int sorted_ok = heap_sort(s1, 9) == 0;
    int want[] = {-3, 0, 1, 2, 5, 5, 6, 7, 9};
    for (int i = 0; i < 9; i++) if (s1[i] != want[i]) sorted_ok = 0;
    CHECK("heap_sort sorts nine values", sorted_ok, "got %d %d %d %d %d %d %d %d %d", s1[0], s1[1], s1[2], s1[3], s1[4], s1[5], s1[6], s1[7], s1[8]);
    CHECK("heap_sort accepts n = 0 and 1", heap_sort(s1, 0) == 0 && heap_sort(s1, 1) == 0 && s1[0] == -3, "failed");
    int *big = malloc(5000 * sizeof *big);
    for (int i = 0; i < 5000; i++) big[i] = (int)(rng(&seed) % 3000u) - 1500;
    ok = heap_sort(big, 5000) == 0;
    for (int i = 1; i < 5000; i++) if (big[i - 1] > big[i]) ok = 0;
    CHECK("heap_sort on 5000 random values", ok, "not sorted");
    free(big);
    return 0;
}
