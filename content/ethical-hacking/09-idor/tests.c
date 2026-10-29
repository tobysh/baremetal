static uint32_t rs;
static uint32_t rnd(void) { rs = rs * 1664525u + 1013904223u; return rs >> 8; }
int main(void) {
    int bad = 0, tot = 0, leak = 0; char first[160] = "";
    for (int db = 0; db < 8; db++) {
        rs = 900 + db * 31;
        Shop s; memset(&s, 0, sizeof s); s.n = 6 + rnd() % 10;
        for (int i = 0; i < s.n; i++) { s.inv[i].id = 1000 + i * 3 + rnd() % 3; s.inv[i].owner = 1 + rnd() % 4; snprintf(s.inv[i].text, 32, "inv %d for %d: %u.%02u", s.inv[i].id, s.inv[i].owner, rnd() % 500, rnd() % 100); }
        for (int uid = 1; uid <= 5; uid++) for (int admin = 0; admin < 2; admin++) {
            Caller c = { uid, admin };
            for (int id = 995; id < 1040; id += 1) {
                int exists = -1; for (int i = 0; i < s.n; i++) if (s.inv[i].id == id) exists = i;
                int want = (exists >= 0 && (admin || s.inv[exists].owner == uid)) ? 200 : 404;
                char out[40]; memset(out, '#', sizeof out); out[39] = 0;
                int got = get_invoice(&s, c, id, out, sizeof out);
                tot++;
                if (got != want) { if (!bad) snprintf(first, sizeof first, "uid %d admin %d id %d: expected %d, got %d", uid, admin, id, want, got); bad++; }
                else if (want == 200 && strcmp(out, s.inv[exists].text)) { if (!bad) snprintf(first, sizeof first, "wrong text for id %d", id); bad++; }
                else if (want == 404 && out[0] != '#') leak++;
            }
        }
    }
    CHECK("only the owner or an admin gets an invoice", bad == 0, "%d of %d requests wrong, e.g. %s", bad, tot, first);
    CHECK("a refused request leaves the output untouched", leak == 0, "%d responses wrote to the output buffer", leak);
    {
        Shop s = { { { 5, 1, "a-long-invoice-text-here-1234567" } }, 1 };
        Caller c = { 1, 0 };
        unsigned char blk[24]; memset(blk, 0xA5, sizeof blk);
        int r = get_invoice(&s, c, 5, (char *)blk + 8, 8);
        int ok = r == 200 && blk[15] == 0 && !memcmp((char *)blk + 8, "a-long-", 7);
        for (int i = 0; i < 8; i++) if (blk[i] != 0xA5 || blk[16 + i] != 0xA5) ok = 0;
        CHECK("small output buffers are respected", ok, "the text must be truncated to cap - 1 characters plus a terminator");
        unsigned char z = 0x5A; r = get_invoice(&s, c, 5, (char *)&z, 0);
        CHECK("cap 0 writes nothing", r == 200 && z == 0x5A, "nothing may be written when cap is 0");
    }
    // Enumeration detection.
    int badE = 0, totE = 0; char fe[160] = "";
    for (int t = 0; t < 12; t++) {
        rs = 3000 + t * 17;
        AccessRec log[120]; int n = 40 + rnd() % 60; long ts = 5000;
        for (int i = 0; i < n; i++) {
            ts += rnd() % (t % 3 == 0 ? 25 : 6);
            log[i].ts = ts; log[i].uid = 1 + rnd() % 3; log[i].id = 1000 + rnd() % 50; log[i].status = (rnd() % (t % 2 ? 3 : 8) == 0) ? 404 : 200;
        }
        for (int uid = 1; uid <= 4; uid++) {
            long want = -1;
            for (int i = 0; i < n && want < 0; i++) if (log[i].uid == uid && log[i].status == 404) {
                int c = 0; for (int j = 0; j <= i; j++) if (log[j].uid == uid && log[j].status == 404 && log[j].ts > log[i].ts - 60) c++;
                if (c >= 5) want = log[i].ts;
            }
            long got = flag_enumeration(log, n, uid); totE++;
            if (got != want) { if (!badE) snprintf(fe, sizeof fe, "log %d uid %d: expected %ld, got %ld", t, uid, want, got); badE++; }
        }
    }
    CHECK("enumeration is flagged at the right record", badE == 0, "%d of %d wrong, e.g. %s", badE, totE, fe);
    {
        AccessRec l[5] = { {100,1,1,404},{110,1,2,404},{120,1,3,404},{130,1,4,404},{159,1,5,404} };
        AccessRec m[5] = { {100,1,1,404},{110,1,2,404},{120,1,3,404},{130,1,4,404},{160,1,5,404} };
        CHECK("window edge is exclusive", flag_enumeration(l, 5, 1) == 159 && flag_enumeration(m, 5, 1) == -1, "a 404 exactly 60 s old is outside the window: (t - 60, t]");
    }
    return 0;
}
