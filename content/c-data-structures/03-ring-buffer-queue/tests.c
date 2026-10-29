static unsigned rng(unsigned *s) { *s = *s * 1103515245u + 12345u; return *s >> 16; }

int main(void) {
    Queue q;
    CHECK("q_init rejects capacity 0", q_init(&q, 0) == -1 && q.buf == NULL, "expected -1 and a NULL buffer");
    CHECK("q_init rejects a negative capacity", q_init(&q, -3) == -1 && q.buf == NULL, "expected -1 and a NULL buffer");
    CHECK("q_init accepts capacity 4", q_init(&q, 4) == 0 && q.buf != NULL && q_size(&q) == 0, "failed");

    int out = -9;
    CHECK("popping an empty queue fails", q_pop(&q, &out) == -1 && out == -9, "expected -1, *out untouched");
    CHECK("peeking an empty queue fails", q_peek(&q, &out) == -1 && out == -9, "expected -1, *out untouched");

    CHECK("push 10, 20, 30, 40", q_push(&q, 10) == 0 && q_push(&q, 20) == 0 && q_push(&q, 30) == 0 && q_push(&q, 40) == 0 && q_size(&q) == 4, "size %d", q_size(&q));
    CHECK("pushing into a full queue fails and changes nothing", q_push(&q, 50) == -1 && q_size(&q) == 4, "size %d", q_size(&q));
    CHECK("peek sees the oldest item without removing it", q_peek(&q, &out) == 0 && out == 10 && q_size(&q) == 4, "out %d size %d", out, q_size(&q));
    CHECK("pop returns 10 then 20", q_pop(&q, &out) == 0 && out == 10 && q_pop(&q, &out) == 0 && out == 20 && q_size(&q) == 2, "out %d", out);

    CHECK("pushes after pops wrap around", q_push(&q, 50) == 0 && q_push(&q, 60) == 0 && q_size(&q) == 4, "size %d", q_size(&q));
    CHECK("full again after wrapping", q_push(&q, 70) == -1, "expected -1");
    int expect[] = {30, 40, 50, 60};
    int ok = 1;
    for (int i = 0; i < 4; i++) { if (q_pop(&q, &out) != 0 || out != expect[i]) ok = 0; }
    CHECK("order after wrapping is 30 40 50 60", ok && q_size(&q) == 0, "wrong order or size");
    CHECK("empty again after draining", q_pop(&q, &out) == -1, "expected -1");
    q_free(&q);
    CHECK("q_free clears the buffer pointer", q.buf == NULL && q.len == 0, "buf %p len %d", (void *)q.buf, q.len);

    Queue one;
    q_init(&one, 1);
    ok = 1;
    for (int i = 0; i < 20; i++) {
        if (q_push(&one, i) != 0 || q_push(&one, 99) != -1 || q_pop(&one, &out) != 0 || out != i) ok = 0;
    }
    CHECK("a capacity-1 queue works over 20 push/pop cycles", ok, "failed");
    q_free(&one);

    Queue r;
    q_init(&r, 7);
    int model[7], mhead = 0, mlen = 0;
    unsigned seed = 31337u;
    ok = 1;
    int steps = 0;
    for (; steps < 3000 && ok; steps++) {
        if (rng(&seed) % 2u) {
            int x = (int)(rng(&seed) % 100000u);
            int rc = q_push(&r, x);
            if (mlen == 7) { if (rc != -1) ok = 0; }
            else { if (rc != 0) ok = 0; model[(mhead + mlen) % 7] = x; mlen++; }
        } else {
            int got = -1;
            int rc = q_pop(&r, &got);
            if (mlen == 0) { if (rc != -1) ok = 0; }
            else { if (rc != 0 || got != model[mhead]) ok = 0; mhead = (mhead + 1) % 7; mlen--; }
        }
        if (q_size(&r) != mlen) ok = 0;
    }
    CHECK("3000 random push/pop steps match a reference queue", ok, "diverged at step %d", steps);
    q_free(&r);
    return 0;
}
