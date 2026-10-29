static int ncase(const char *a, const char *b, size_t n) { for (size_t i = 0; i < n; i++) { int x = tolower((unsigned char)a[i]), y = tolower((unsigned char)b[i]); if (x != y) return x - y; if (!x) return 0; } return 0; }
typedef struct { const char *name[40]; int id[40]; int n; int text_calls, param_calls; } Toy;

static int match_all(Toy *t, const char *v, int *ids, int cap, int *k) { (void)v; for (int i = 0; i < t->n && *k < cap; i++) ids[(*k)++] = t->id[i]; return *k; }
static int toy_text(void *db, const char *sql, int *ids, int cap) {
    Toy *t = db; t->text_calls++;
    const char *pre = "SELECT id FROM users WHERE name='";
    if (strncmp(sql, pre, strlen(pre))) return -1;
    const char *v = sql + strlen(pre), *q = strchr(v, '\'');
    if (!q) return -1;
    char val[200]; size_t vl = (size_t)(q - v); if (vl >= sizeof val) return -1;
    memcpy(val, v, vl); val[vl] = 0;
    const char *rest = q + 1; int k = 0;
    if (!ncase(rest, " OR ", 4)) return match_all(t, val, ids, cap, &k);
    int unionrow = !ncase(rest, " UNION", 6);
    if (*rest && !unionrow && strncmp(rest, "--", 2) && *rest != ';') return -1;
    for (int i = 0; i < t->n && k < cap; i++) if (!strcmp(t->name[i], val)) ids[k++] = t->id[i];
    if (unionrow && k < cap) ids[k++] = 9999;
    return k;
}
static int toy_param(void *db, const char *sql, const char *p, int *ids, int cap) {
    Toy *t = db; t->param_calls++;
    if (strcmp(sql, "SELECT id FROM users WHERE name = ?")) return -1;
    int k = 0;
    for (int i = 0; i < t->n && k < cap; i++) if (!strcmp(t->name[i], p)) ids[k++] = t->id[i];
    return k;
}
static int has_ci(const char *s, const char *pat) {
    size_t n = strlen(pat);
    for (; *s; s++) { size_t i = 0; while (i < n && s[i] && tolower((unsigned char)s[i]) == pat[i]) i++; if (i == n) return 1; }
    return 0;
}
static int ref_inj(const char *s) { return strchr(s, '\'') || strstr(s, "--") || strchr(s, ';') || strstr(s, "/*") || has_ci(s, " or ") || has_ci(s, " union "); }

int main(void) {
    DbApi api = { toy_text, toy_param };
    static const char *pool[] = { "alice", "bob", "O'Brien", "carol", "x", "dave", "alice", "100%", "eve;", "a--b", "Alice" };
    int tot = 0, bad = 0, textbad = 0; char first[200] = "";
    for (int db = 0; db < 6; db++) {
        Toy t; memset(&t, 0, sizeof t);
        t.n = 4 + db * 3;
        for (int i = 0; i < t.n; i++) { t.name[i] = pool[(i * 5 + db * 3) % 11]; t.id[i] = 100 * (db + 1) + i; }
        static const char *inputs[] = { "alice", "bob", "O'Brien", "nobody", "", "x' OR '1'='1", "' OR 1=1 --", "alice'--", "zz' UNION SELECT secret", "bob'; DROP TABLE users", "100%", "Alice", "carol" };
        for (int q = 0; q < 13; q++) {
            int want[40], wn = 0;
            for (int i = 0; i < t.n; i++) if (!strcmp(t.name[i], inputs[q]) && wn < 5) want[wn++] = t.id[i];
            int ids[40]; for (int i = 0; i < 40; i++) ids[i] = -1;
            int before = t.text_calls;
            int got = find_user_ids(&api, &t, inputs[q], ids, 5);
            tot++;
            if (t.text_calls != before) textbad++;
            int ok = got == wn;
            for (int i = 0; ok && i < wn; i++) if (ids[i] != want[i]) ok = 0;
            if (!ok) { if (!bad) snprintf(first, sizeof first, "name [%s] in database %d: expected %d ids, got %d", inputs[q], db, wn, got); bad++; }
        }
    }
    CHECK("lookups return exactly the matching users", bad == 0, "%d of %d lookups wrong, e.g. %s", bad, tot, first);
    CHECK("the text interface is never used", textbad == 0, "exec_text was called %d times, build no SQL strings", textbad);
    {
        Toy t; memset(&t, 0, sizeof t); t.n = 2; t.name[0] = "O'Brien"; t.id[0] = 7; t.name[1] = "x' OR '1'='1"; t.id[1] = 8;
        int ids[4] = { -1, -1, -1, -1 };
        int g = find_user_ids(&api, &t, "O'Brien", ids, 4);
        CHECK("an honest name with a quote still works", g == 1 && ids[0] == 7, "expected id 7, got %d results", g);
        g = find_user_ids(&api, &t, "x' OR '1'='1", ids, 4);
        CHECK("the attack string is just a name", g == 1 && ids[0] == 8, "only the user literally named that should match, got %d results", g);
        CHECK("NULL name finds nothing", find_user_ids(&api, &t, NULL, ids, 4) == 0, "NULL must return 0");
        CHECK("cap is respected", find_user_ids(&api, &t, "O'Brien", ids, 0) == 0, "cap 0 must store nothing");
    }
    {
        static const char *in[] = { "alice", "O'Brien", "a--b", "x; y", "/* c */", "1 OR 1", "1 or 1", "a Or b", "UNION SELECT", "a union select", "a UnIoN b", "order", "color", "fork", "format", "a or", "or b", "", "Smith-Jones", "a-b", "100%", "a/b*c" };
        int bad2 = 0; char f2[120] = "";
        for (size_t i = 0; i < sizeof in / sizeof in[0]; i++) if (!!looks_like_injection(in[i]) != !!ref_inj(in[i])) { if (!bad2) snprintf(f2, sizeof f2, "[%s] expected %d", in[i], ref_inj(in[i])); bad2++; }
        CHECK("tripwire flags the right inputs", bad2 == 0 && looks_like_injection(NULL) == 0, "%d wrong, e.g. %s", bad2, f2);
    }
    return 0;
}
