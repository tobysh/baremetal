static uint32_t st = 8080;
static uint32_t rnd(void) { st = st * 1664525u + 1013904223u; return st >> 8; }

static const char REQ[] = "GET /notes HTTP/1.1\r\nHost: example.test\r\n\r\n";

int main(void)
{
    const uint8_t *req = (const uint8_t *)REQ;
    const size_t rn = sizeof REQ - 1;
    char out[64];

    /* Two segments at a random cut, delivered in both orders, plus a duplicate. */
    for (int round = 0; round < 6; round++) {
        size_t k = 1 + rnd() % (rn - 1);
        uint32_t base = rnd() % 100000;
        Segment s[] = {{base + (uint32_t)k, req + k, rn - k}, {base, req, k}, {base, req, k}};
        memset(out, 'Z', sizeof out);
        int rc = http_path(s, 3, base, out, sizeof out);
        CHECK("out-of-order and duplicate segments", rc == 6 && !strcmp(out, "/notes"), "cut at %zu, base %u: returned %d, out \"%.10s\" (want 6 and /notes)", k, base, rc, out);
    }

    {
        Segment s[sizeof REQ - 1];
        size_t count = 0;
        for (size_t i = rn; i-- > 0;)
            s[count++] = (Segment){2000 + (uint32_t)i, req + i, 1};
        int rc = http_path(s, count, 2000, out, sizeof out);
        CHECK("one byte per segment, reversed", rc == 6 && !strcmp(out, "/notes"), "reassembling %zu single-byte segments gave %d", count, rc);
    }

    {
        const uint8_t other[] = "GET /a/b?q=7&z=%20 HTTP/1.1\r\n\r\n";
        Segment s = {9, other, sizeof other - 1};
        int rc = http_path(&s, 1, 9, out, sizeof out);
        CHECK("another path and an empty header block", rc == 14 && !strcmp(out, "/a/b?q=7&z=%20"), "got %d \"%s\"; the path is everything up to the next space", rc, out);
        const uint8_t root[] = "GET / HTTP/1.1\r\n\r\n";
        Segment r = {0, root, sizeof root - 1};
        rc = http_path(&r, 1, 0, out, sizeof out);
        CHECK("shortest valid request", rc == 1 && !strcmp(out, "/"), "an 18-byte request with path / returned %d", rc);
    }

    {
        const uint8_t body[] = "GET /x HTTP/1.1\r\nHost: a\r\n\r\nbody=1";
        Segment s = {5, body, sizeof body - 1};
        int rc = http_path(&s, 1, 5, out, sizeof out);
        CHECK("bytes after the headers are ignored", rc == 2 && !strcmp(out, "/x"), "a body after the blank line must not matter, got %d", rc);
    }

    {
        Segment gap[] = {{1001, req + 1, rn - 1}};
        int first = http_path(gap, 1, 1000, out, sizeof out);
        Segment mid[] = {{1000, req, 10}, {1012, req + 12, rn - 12}};
        int middle = http_path(mid, 2, 1000, out, sizeof out);
        CHECK("gaps are rejected", first == -1 && middle == -1, "missing byte 0 gave %d, missing bytes 10-11 gave %d (want -1 for both)", first, middle);
    }

    {
        uint8_t changed[sizeof REQ - 1];
        memcpy(changed, req, rn);
        changed[5] ^= 1;
        Segment s[] = {{1000, req, rn}, {1000, changed, rn}};
        int rc = http_path(s, 2, 1000, out, sizeof out);
        Segment same[] = {{1000, req, rn}, {1000, req, rn}};
        int ok = http_path(same, 2, 1000, out, sizeof out);
        CHECK("conflicting overlap", rc == -1 && ok == 6, "one differing byte in an overlap must fail (%d); an identical copy must pass (%d)", rc, ok);
    }

    {
        size_t half = 22;
        Segment partial = {1000, req, half};
        int cut = http_path(&partial, 1, 1000, out, sizeof out);
        const uint8_t nohdr[] = "GET /x HTTP/1.1\r\nHost: a\r\n";
        Segment h = {1, nohdr, sizeof nohdr - 1};
        int unfinished = http_path(&h, 1, 1, out, sizeof out);
        CHECK("headers must be complete", cut == -1 && unfinished == -1, "no blank line yet: got %d and %d (want -1 for both)", cut, unfinished);
    }

    {
        const uint8_t post[] = "POST /x HTTP/1.1\r\n\r\n";
        const uint8_t v10[] = "GET /x HTTP/1.0\r\n\r\n";
        const uint8_t nopath[] = "GET  HTTP/1.1\r\n\r\n";
        const uint8_t lower[] = "get /x HTTP/1.1\r\n\r\n";
        Segment a = {1, post, sizeof post - 1}, b = {1, v10, sizeof v10 - 1}, c = {1, nopath, sizeof nopath - 1}, d = {1, lower, sizeof lower - 1};
        int ra = http_path(&a, 1, 1, out, sizeof out), rb = http_path(&b, 1, 1, out, sizeof out);
        int rc = http_path(&c, 1, 1, out, sizeof out), rd = http_path(&d, 1, 1, out, sizeof out);
        CHECK("only GET with HTTP/1.1 and a path", ra == -1 && rb == -1 && rc == -1 && rd == -1, "POST %d, HTTP/1.0 %d, empty path %d, lowercase get %d (want -1 for all)", ra, rb, rc, rd);
    }

    {
        const uint8_t ctl[] = "GET /x HTTP/1.1\r\nX: a\x01" "b\r\n\r\n";
        Segment s = {1, ctl, sizeof ctl - 1};
        int rc = http_path(&s, 1, 1, out, sizeof out);
        CHECK("control bytes are rejected", rc == -1, "a 0x01 byte in the headers must fail, got %d", rc);
    }

    {
        Segment s = {1000, req, rn};
        memset(out, 'Z', sizeof out);
        int exact = http_path(&s, 1, 1000, out, 7);
        int tight = http_path(&s, 1, 1000, out, 6);
        CHECK("output capacity", exact == 6 && !strcmp(out, "/notes") && tight == -1, "cap 7 holds /notes plus the NUL (got %d); cap 6 does not (got %d)", exact, tight);
        memset(out, 'Z', sizeof out);
        http_path(&s, 1, 1000, out, 3);
        CHECK("failed call leaves out unchanged", out[0] == 'Z' && out[1] == 'Z' && out[2] == 'Z', "out was modified although the call failed");
    }

    {
        static uint8_t big[1100];
        memset(big, 'a', sizeof big);
        int hl = snprintf((char *)big, 60, "GET /p HTTP/1.1\r\nX: ");
        memset(big + hl, 'a', 1024 - hl - 4);
        memcpy(big + 1024 - 4, "\r\n\r\n", 4);
        Segment fit = {500, big, 1024};
        int ok = http_path(&fit, 1, 500, out, sizeof out);
        big[1024] = 'a';
        Segment over = {500, big, 1025};
        int bad = http_path(&over, 1, 500, out, sizeof out);
        Segment far = {500 + 5000, big, 20};
        int beyond = http_path(&far, 1, 500, out, sizeof out);
        Segment early = {499, big, 20};
        int before = http_path(&early, 1, 500, out, sizeof out);
        CHECK("1024-byte window", ok == 2 && bad == -1 && beyond == -1 && before == -1, "1024 bytes: %d (want 2); 1025 bytes: %d; a segment far past the window: %d; before base: %d (want -1 each)", ok, bad, beyond, before);
    }

    {
        Segment s = {1000, req, rn};
        Segment withnull[] = {{1000, req, rn}, {1000, NULL, 3}};
        Segment zero = {1000, NULL, 0};
        Segment zs[] = {{4000, req, 0}, {1000, req, rn}};
        CHECK("NULL data and empty segments", http_path(withnull, 2, 1000, out, sizeof out) == -1 && http_path(&zero, 1, 1000, out, sizeof out) == -1 && http_path(zs, 2, 1000, out, sizeof out) == 6, "a NULL pointer with a positive length fails; zero-length segments are ignored, even far outside the window");
        CHECK("empty stream and bad output", http_path(NULL, 0, 1000, out, sizeof out) == -1 && http_path(&s, 1, 1000, NULL, 20) == -1 && http_path(&s, 1, 1000, out, 0) == -1, "no segments, NULL out and cap 0 must all return -1");
    }
    return 0;
}
