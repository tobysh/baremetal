// Builds each record by hand so the checks do not depend on record_init.
static void fresh(record *r) {
    memset(r, 0, sizeof *r);
    memset(r->guard_lo, 0xAA, 4);
    memset(r->guard_hi, 0xAA, 4);
    memset(r->name, 0x7E, sizeof r->name);   // sentinel: unused bytes must survive
}

static int guards_ok(const record *r) {
    for (int i = 0; i < 4; i++) {
        if (r->guard_lo[i] != 0xAA || r->guard_hi[i] != 0xAA) return 0;
    }
    return 1;
}

static int try_len(int len, int *dropped, record *r) {
    char buf[64];
    for (int i = 0; i < len; i++) buf[i] = (char)('a' + i % 26);
    buf[len] = '\0';
    fresh(r);
    *dropped = set_name(r, buf);
    return strncmp(r->name, buf, len < 7 ? len : 7) == 0;
}

int main(void) {
    record r;
    int d;

    int ok = try_len(3, &d, &r);
    CHECK("short names are stored", ok && strcmp(r.name, "abc") == 0, "name is '%s', expected 'abc'", r.name);
    CHECK("a short name drops nothing", d == 0, "returned %d, expected 0", d);
    CHECK("a short name leaves both guards alone", guards_ok(&r), "a guard byte changed");
    CHECK("bytes after the terminator are not touched", (unsigned char)r.name[4] == 0x7E, "name[4] is 0x%02X, expected the sentinel 0x7E", (unsigned char)r.name[4]);

    try_len(0, &d, &r);
    CHECK("an empty name gives an empty string", r.name[0] == '\0' && d == 0 && guards_ok(&r), "name[0]=%d, returned %d", r.name[0], d);

    ok = try_len(7, &d, &r);
    CHECK("7 characters fit exactly", ok && strcmp(r.name, "abcdefg") == 0 && d == 0, "name '%s', returned %d", r.name, d);
    CHECK("7 characters leave the guards alone", guards_ok(&r), "a guard byte changed");

    ok = try_len(8, &d, &r);
    CHECK("8 characters lose exactly one", d == 1, "returned %d, expected 1", d);
    CHECK("8 characters are cut to 7 and terminated", ok && strcmp(r.name, "abcdefg") == 0, "name is '%s', expected 'abcdefg'", r.name);
    CHECK("8 characters do not touch the upper guard", guards_ok(&r), "guard_hi[0] is 0x%02X, expected 0xAA", r.guard_hi[0]);

    ok = try_len(40, &d, &r);
    CHECK("a very long input drops all but 7", d == 33, "returned %d, expected 33", d);
    CHECK("a very long input is still a valid string", ok && strlen(r.name) == 7, "strlen is %zu, expected 7", strlen(r.name));
    CHECK("a very long input keeps both guards", guards_ok(&r), "a guard byte changed");

    fresh(&r);
    d = set_name(&r, "Bartholomew");
    CHECK("the bug report's name is truncated to Barthol", strcmp(r.name, "Bartholo") != 0 && strcmp(r.name, "Barthol") == 0 && d == 4,
          "name '%s', returned %d, expected 'Barthol' and 4", r.name, d);
    CHECK("the bug report's guards are intact", guards_ok(&r), "a guard byte changed");
    return 0;
}
