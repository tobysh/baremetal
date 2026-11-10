static uint32_t st = 2718;
static uint32_t rnd(void) { st = st * 1664525u + 1013904223u; return st >> 8; }

static void w16(uint8_t *p, unsigned v) { p[0] = (uint8_t)(v >> 8); p[1] = (uint8_t)v; }

/* Header with the given id and QDCOUNT; returns 12. */
static size_t head(uint8_t *b, unsigned id, unsigned qd)
{
    memset(b, 0, 12);
    w16(b, id);
    b[2] = 0x01;
    w16(b + 4, qd);
    return 12;
}

/* Encodes a dotted name at b+off and appends QTYPE/QCLASS. Returns the new length. */
static size_t question(uint8_t *b, size_t off, const char *name, unsigned type, unsigned qclass)
{
    if (strcmp(name, ".")) {
        while (*name) {
            const char *dot = strchr(name, '.');
            size_t len = dot ? (size_t)(dot - name) : strlen(name);
            b[off++] = (uint8_t)len;
            memcpy(b + off, name, len);
            off += len;
            name += len + (dot ? 1 : 0);
        }
    }
    b[off++] = 0;
    w16(b + off, type); w16(b + off + 2, qclass);
    return off + 4;
}

/* A dotted name made of labels of the given lengths, each filled with a random letter. */
static void labels(char *s, const size_t *lens, size_t count)
{
    size_t k = 0;
    for (size_t i = 0; i < count; i++) {
        if (i) s[k++] = '.';
        for (size_t j = 0; j < lens[i]; j++) s[k++] = (char)('a' + rnd() % 26);
    }
    s[k] = 0;
}

int main(void)
{
    uint8_t b[600];
    Question q;
    Question sentinel;
    memset(&sentinel, 0x5a, sizeof sentinel);

    {
        size_t n = question(b, head(b, 0x1234, 1), "notes.example.test", 1, 1);
        memset(&q, 0, sizeof q);
        int rc = dns_question(b, n, &q);
        CHECK("ordinary question", rc == 0 && q.id == 0x1234 && q.type == 1 && q.qclass == 1 && !strcmp(q.name, "notes.example.test"), "rc %d id 0x%04x type %u class %u name \"%s\"", rc, q.id, q.type, q.qclass, q.name);
    }

    {
        static const char *names[] = {"WwW.Example.TEST", "a", "x-y.9z.test", "a1-b2.c3-d4.e5.test"};
        int bad = -1;
        for (int i = 0; i < 4; i++) {
            unsigned t = 1 + rnd() % 60000, c = 1 + rnd() % 255;
            size_t n = question(b, head(b, rnd() & 0xffff, 1), names[i], t, c);
            memset(&q, 0, sizeof q);
            if (dns_question(b, n, &q) || strcmp(q.name, names[i]) || q.type != t || q.qclass != c) { bad = i; break; }
        }
        CHECK("case digits hyphens and varied type and class", bad < 0, "name \"%s\" was not decoded exactly (case must be preserved; type and class are big-endian)", bad < 0 ? "" : names[bad]);
    }

    {
        size_t n = question(b, head(b, 7, 1), ".", 28, 1);
        memset(&q, 0, sizeof q);
        int rc = dns_question(b, n, &q);
        CHECK("root name is a dot", rc == 0 && !strcmp(q.name, ".") && q.type == 28, "a name that is only the zero byte should display as \".\", got rc %d \"%s\"", rc, q.name);
    }

    {
        size_t n = question(b, head(b, 9, 1), "a.test", 1, 1);
        for (int i = 0; i < 30; i++) b[n++] = (uint8_t)rnd();
        memset(&q, 0, sizeof q);
        int rc = dns_question(b, n, &q);
        CHECK("trailing answer bytes are ignored", rc == 0 && !strcmp(q.name, "a.test"), "bytes after QCLASS must not affect the question, rc %d", rc);
    }

    {
        int c0, c2;
        size_t n = question(b, head(b, 1, 0), "a.test", 1, 1); c0 = dns_question(b, n, &q);
        n = question(b, head(b, 1, 2), "a.test", 1, 1); c2 = dns_question(b, n, &q);
        CHECK("QDCOUNT must be exactly 1", c0 == -1 && c2 == -1, "QDCOUNT 0 and 2 must be rejected, got %d %d", c0, c2);
    }

    {
        size_t n = head(b, 1, 1);
        b[n++] = 3; b[n++] = 'w'; b[n++] = 'w'; b[n++] = 'w';
        b[n++] = 0xc0; b[n++] = 0x0c;                 /* pointer to offset 12 */
        w16(b + n, 1); w16(b + n + 2, 1); n += 4;
        int ptr = dns_question(b, n, &q);
        n = head(b, 1, 1);
        b[n++] = 0xc0; b[n++] = 0x0c; w16(b + n, 1); w16(b + n + 2, 1); n += 4;
        int first = dns_question(b, n, &q);
        CHECK("compression pointers are rejected", ptr == -1 && first == -1, "a length byte of 0xc0 is a pointer, not a 192-byte label (got %d, %d)", ptr, first);
    }

    {
        char name[300];
        size_t ok63[] = {63, 4};
        size_t bad64[] = {64, 4};
        labels(name, ok63, 2);
        size_t n = question(b, head(b, 1, 1), name, 1, 1);
        int a = dns_question(b, n, &q);
        labels(name, bad64, 2);
        n = question(b, head(b, 1, 1), name, 1, 1);
        int c = dns_question(b, n, &q);
        CHECK("label length limit is 63", a == 0 && c == -1, "a 63-byte label must pass (got %d) and a 64-byte one must fail (got %d)", a, c);
    }

    {
        char name[300];
        size_t ok253[] = {63, 63, 63, 61};    /* 63+63+63+61 plus three dots = 253 */
        size_t bad254[] = {63, 63, 63, 62};
        labels(name, ok253, 4);
        size_t n = question(b, head(b, 1, 1), name, 1, 1);
        memset(&q, 0, sizeof q);
        int a = dns_question(b, n, &q);
        int same = !strcmp(q.name, name);
        labels(name, bad254, 4);
        n = question(b, head(b, 1, 1), name, 1, 1);
        int c = dns_question(b, n, &q);
        CHECK("display name limit is 253", a == 0 && same && c == -1, "253 characters must pass and decode exactly (got %d, same %d); 254 must fail (got %d)", a, same, c);
    }

    {
        size_t base = question(b, head(b, 1, 1), "notes.example.test", 1, 1);
        int bad = -1;
        for (size_t cut = 12; cut < base; cut++)
            if (dns_question(b, cut, &q) != -1) { bad = (int)cut; break; }
        CHECK("every truncation is rejected", bad < 0, "a message cut to %d bytes (inside the name, or missing QTYPE/QCLASS) must return -1", bad);
    }

    {
        size_t n = head(b, 1, 1);
        b[n++] = 5; memcpy(b + n, "no_es", 5); n += 5;
        b[n++] = 0; w16(b + n, 1); w16(b + n + 2, 1); n += 4;
        int under = dns_question(b, n, &q);
        n = head(b, 1, 1);
        b[n++] = 2; b[n++] = 'a'; b[n++] = ' '; b[n++] = 0; w16(b + n, 1); w16(b + n + 2, 1); n += 4;
        int space = dns_question(b, n, &q);
        CHECK("only letters digits and hyphens", under == -1 && space == -1, "underscore and space in a label must be rejected, got %d %d", under, space);
    }

    {
        q = sentinel;
        size_t n = question(b, head(b, 1, 1), "a.test", 1, 1);
        dns_question(b, n - 1, &q);
        int same = !memcmp(&q, &sentinel, sizeof q);
        dns_question(b, 5, &q);
        CHECK("failure leaves out unchanged", same && !memcmp(&q, &sentinel, sizeof q), "a failed call wrote to *out (a parse that fails late must not leave a half-written name)");
    }

    CHECK("NULL arguments and short header", dns_question(NULL, 40, &q) == -1 && dns_question(b, 40, NULL) == -1 && dns_question(b, 11, &q) == -1, "NULL pointers and n < 12 must return -1");
    return 0;
}
