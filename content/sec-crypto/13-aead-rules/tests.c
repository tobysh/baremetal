static uint32_t rs = 4242;
static uint32_t rnd(void) { rs ^= rs << 13; rs ^= rs >> 17; rs ^= rs << 5; return rs; }
static int ref_audit(const Call *log, int n, int *kind) {
    for (int i = 0; i < n; i++) {
        if (log[i].op == 'S') {
            for (int j = 0; j < i; j++) if (log[j].op == 'S' && log[j].key == log[i].key && log[j].nonce == log[i].nonce) { *kind = 1; return i; }
        } else if (!log[i].tag_first) { *kind = 2; return i; }
        if (!log[i].aad_bound) { *kind = 3; return i; }
    }
    *kind = 0; return -1;
}
int main(void) {
    // Fresh nonces, limit.
    {
        Channel ch = { 0xABCDEF, 5, 8 };
        Packet a, b, c, d; memset(&d, 0x77, sizeof d);
        int r1 = channel_seal(&ch, "a", (const uint8_t *)"same text", 9, &a);
        int r2 = channel_seal(&ch, "a", (const uint8_t *)"same text", 9, &b);
        int r3 = channel_seal(&ch, "a", (const uint8_t *)"same text", 9, &c);
        CHECK("the nonce comes from the counter", r1 == 0 && r2 == 0 && r3 == 0 && a.nonce == 5 && b.nonce == 6 && c.nonce == 7 && ch.counter == 8,
              "expected nonces 5, 6, 7 and a counter of 8, got %u %u %u and %u", a.nonce, b.nonce, c.nonce, ch.counter);
        CHECK("identical messages give different ciphertexts", memcmp(a.ct, b.ct, 9) != 0 && memcmp(b.ct, c.ct, 9) != 0, "a reused nonce leaks that two messages are equal");
        int r4 = channel_seal(&ch, "a", (const uint8_t *)"same text", 9, &d);
        unsigned char *q = (unsigned char *)&d; int touched = 0; for (size_t i = 0; i < sizeof d; i++) if (q[i] != 0x77) touched = 1;
        CHECK("the channel refuses to wrap its counter", r4 == -1 && ch.counter == 8 && !touched, "at the limit seal must return -1 and change nothing");
        Channel ch2 = { 9, 0, 100 }; Packet e;
        CHECK("messages over 64 bytes are refused", channel_seal(&ch2, "a", (const uint8_t *)"x", 65, &e) == -1 && ch2.counter == 0, "return -1 without spending a nonce");
    }
    // Round trips and tag contents.
    {
        int bad = 0, badtag = 0, tot = 0; char first[120] = "";
        for (int t = 0; t < 60; t++) {
            Channel ch = { ((uint64_t)rnd() << 32) | rnd(), rnd() % 1000, 100000 };
            char aad[24]; snprintf(aad, sizeof aad, "user=%u;ep=%u", rnd() % 90, rnd() % 5);
            size_t n = t < 3 ? (size_t)(t * 32) : rnd() % 65;
            uint8_t pt[64], out[64]; for (size_t i = 0; i < n; i++) pt[i] = (uint8_t)rnd();
            Packet p; uint32_t c0 = ch.counter;
            tot++;
            if (channel_seal(&ch, aad, pt, n, &p) != 0 || p.nonce != c0) { if (!bad) snprintf(first, sizeof first, "seal failed at trial %d", t); bad++; continue; }
            if (p.tag != toy_tag(ch.key, p.nonce, aad, p.ct, n)) badtag++;
            memset(out, 0, sizeof out);
            int r = channel_open(&ch, aad, &p, out, sizeof out);
            if (r != (int)n || (n && memcmp(out, pt, n))) { if (!bad) snprintf(first, sizeof first, "round trip failed at trial %d (n=%zu, r=%d)", t, n, r); bad++; }
        }
        CHECK("seal then open returns the message", bad == 0, "%d of %d failed, e.g. %s", bad, tot, first);
        CHECK("the tag covers key, nonce, aad and ciphertext", badtag == 0, "%d tags differ from toy_tag(key, nonce, aad, ct, n)", badtag);
    }
    // Rejections leave the output alone.
    {
        Channel ch = { 0x5151, 40, 1000 }; Packet p;
        const uint8_t msg[] = "wire 100 to account 55";
        channel_seal(&ch, "from=alice", msg, sizeof msg - 1, &p);
        struct { const char *name; const char *aad; Packet pk; size_t cap; } T[6];
        for (int i = 0; i < 6; i++) { T[i].aad = "from=alice"; T[i].pk = p; T[i].cap = 64; }
        T[0].name = "a flipped ciphertext bit"; T[0].pk.ct[3] ^= 1;
        T[1].name = "a flipped tag bit"; T[1].pk.tag ^= 1ULL << 40;
        T[2].name = "a changed nonce"; T[2].pk.nonce ^= 2;
        T[3].name = "different associated data"; T[3].aad = "from=mallory";
        T[4].name = "a shortened packet"; T[4].pk.n -= 1;
        T[5].name = "an output buffer that is too small"; T[5].cap = 8;
        for (int i = 0; i < 6; i++) {
            uint8_t out[64]; memset(out, 0xC3, sizeof out);
            int r = channel_open(&ch, T[i].aad, &T[i].pk, out, T[i].cap);
            int clean = 1; for (size_t j = 0; j < sizeof out; j++) if (out[j] != 0xC3) clean = 0;
            char name[80]; snprintf(name, sizeof name, "rejects %s", T[i].name);
            CHECK(name, r == -1, "open returned %d", r);
            snprintf(name, sizeof name, "no plaintext written for %s", T[i].name);
            CHECK(name, clean, "the output buffer was modified before or despite the failed check");
        }
        uint8_t out[64]; Packet big = p; big.n = 70;
        CHECK("rejects an oversized length field", channel_open(&ch, "from=alice", &big, out, sizeof out) == -1, "n over 64 must fail");
        Channel other = { 0x5152, 40, 1000 };
        CHECK("rejects a different key", channel_open(&other, "from=alice", &p, out, sizeof out) == -1, "wrong key");
        Packet z; Channel cz = { 77, 0, 10 };
        channel_seal(&cz, "", (const uint8_t *)"", 0, &z);
        CHECK("empty messages work", channel_open(&cz, "", &z, out, 0) == 0, "an empty message is valid");
    }
    // Log audit.
    {
        int bad = 0, tot = 0; char first[140] = "";
        for (int t = 0; t < 80; t++) {
            Call log[40]; int n = 4 + rnd() % 30; int pbad = t % 4 == 0 ? 0 : 12 + (int)(rnd() % 40);
            uint32_t next[3] = { 0, 0, 0 };
            for (int i = 0; i < n; i++) {
                log[i].op = (rnd() % 3) ? 'S' : 'O';
                log[i].key = 1 + rnd() % 3;
                log[i].nonce = next[log[i].key - 1]++;
                log[i].aad_bound = 1; log[i].tag_first = 1;
                if (pbad && rnd() % pbad == 0) {
                    switch (rnd() % 3) {
                    case 0: if (log[i].op == 'S' && next[log[i].key - 1] > 1) log[i].nonce = rnd() % (next[log[i].key - 1] - 1); break;
                    case 1: log[i].tag_first = 0; break;
                    default: log[i].aad_bound = 0; break;
                    }
                }
            }
            int k1 = -9, k2 = -9;
            int want = ref_audit(log, n, &k1), got = audit_calls(log, n, &k2);
            tot++;
            if (want != got || k1 != k2) { if (!bad) snprintf(first, sizeof first, "log %d: expected index %d kind %d, got index %d kind %d", t, want, k1, got, k2); bad++; }
        }
        CHECK("the audit finds the first misuse and names it", bad == 0, "%d of %d logs wrong, e.g. %s", bad, tot, first);
        Call same_nonce_other_key[3] = { {'S',1,7,1,1}, {'S',2,7,1,1}, {'O',1,7,1,1} };
        int k = -1;
        CHECK("the same nonce under another key is fine", audit_calls(same_nonce_other_key, 3, &k) == -1 && k == 0, "nonces only collide within one key, and opens do not count");
        Call both[2] = { {'O',1,0,1,0}, {'S',1,0,0,1} };
        CHECK("tag-before-decrypt outranks aad on the same call", audit_calls(both, 2, &k) == 0 && k == 2, "report kind 2 for index 0");
    }
    return 0;
}
