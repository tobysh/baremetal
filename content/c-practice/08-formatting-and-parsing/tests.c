static Pair mk(const char *k, const char *v) {
    Pair p;
    memset(&p, 0, sizeof p);
    strcpy(p.key, k);
    strcpy(p.value, v);
    return p;
}

int main(void) {
    char buf[32];
    CHECK("format_time 3661 is 01:01:01", format_time(buf, sizeof buf, 3661) == 0 && strcmp(buf, "01:01:01") == 0, "got \"%s\"", buf);
    CHECK("format_time 0 is 00:00:00", format_time(buf, sizeof buf, 0) == 0 && strcmp(buf, "00:00:00") == 0, "got \"%s\"", buf);
    CHECK("format_time 59 seconds is 00:00:59", format_time(buf, sizeof buf, 59) == 0 && strcmp(buf, "00:00:59") == 0, "got \"%s\"", buf);
    CHECK("format_time 86399 is 23:59:59", format_time(buf, sizeof buf, 86399) == 0 && strcmp(buf, "23:59:59") == 0, "got \"%s\"", buf);
    CHECK("format_time allows three-digit hours", format_time(buf, sizeof buf, 360000) == 0 && strcmp(buf, "100:00:00") == 0, "got \"%s\"", buf);
    strcpy(buf, "junk");
    CHECK("format_time rejects negative seconds and clears the buffer", format_time(buf, sizeof buf, -5) == -1 && buf[0] == '\0', "buf = \"%s\"", buf);
    char exact[9];
    CHECK("format_time fits when the buffer is exactly big enough (9 bytes)", format_time(exact, sizeof exact, 3661) == 0 && strcmp(exact, "01:01:01") == 0, "got \"%s\"", exact);
    char tiny[6];
    int r = format_time(tiny, sizeof tiny, 3661);
    CHECK("format_time reports truncation and keeps a valid string", r == -1 && strcmp(tiny, "01:01") == 0, "returned %d, buffer = \"%s\"", r, tiny);
    CHECK("format_time with cap 0 returns -1 without writing", format_time(NULL, 0, 5) == -1, "expected -1");

    Pair ps[3] = { mk("a", "1"), mk("name", "ada"), mk("e", "") };
    r = format_pairs(buf, sizeof buf, ps, 2);
    CHECK("format_pairs joins pairs as key=value;", r == 13 && strcmp(buf, "a=1;name=ada;") == 0, "returned %d, got \"%s\"", r, buf);
    r = format_pairs(buf, sizeof buf, ps, 3);
    CHECK("format_pairs handles an empty value", r == 16 && strcmp(buf, "a=1;name=ada;e=;") == 0, "returned %d, got \"%s\"", r, buf);
    strcpy(buf, "junk");
    r = format_pairs(buf, sizeof buf, ps, 0);
    CHECK("format_pairs with no pairs gives an empty string and 0", r == 0 && buf[0] == '\0', "returned %d, got \"%s\"", r, buf);
    char small[8];
    r = format_pairs(small, sizeof small, ps, 2);
    CHECK("format_pairs returns -1 when it does not fit, leaving a valid string", r == -1 && strlen(small) < sizeof small, "returned %d", r);
    char exactp[5];
    r = format_pairs(exactp, sizeof exactp, ps, 1);
    CHECK("format_pairs fits when exactly big enough", r == 4 && strcmp(exactp, "a=1;") == 0, "returned %d, got \"%s\"", r, exactp);
    char nofit[4];
    r = format_pairs(nofit, sizeof nofit, ps, 1);
    CHECK("format_pairs needs room for the final zero byte", r == -1, "returned %d, expected -1", r);
    CHECK("format_pairs with cap 0 returns -1", format_pairs(NULL, 0, ps, 0) == -1, "expected -1");

    Pair out[4];
    r = parse_pairs("name=ada;age=36;", out, 4);
    CHECK("parse_pairs reads two pairs", r == 2 && strcmp(out[0].key, "name") == 0 && strcmp(out[0].value, "ada") == 0 && strcmp(out[1].key, "age") == 0 && strcmp(out[1].value, "36") == 0, "returned %d", r);
    r = parse_pairs("x=1;y=2", out, 4);
    CHECK("the final semicolon is optional", r == 2 && strcmp(out[1].key, "y") == 0 && strcmp(out[1].value, "2") == 0, "returned %d", r);
    r = parse_pairs("flag=;", out, 4);
    CHECK("a value may be empty", r == 1 && strcmp(out[0].key, "flag") == 0 && out[0].value[0] == '\0', "returned %d", r);
    r = parse_pairs("flag=", out, 4);
    CHECK("an empty value at the very end is fine", r == 1 && out[0].value[0] == '\0', "returned %d", r);
    CHECK("empty text has zero pairs", parse_pairs("", out, 4) == 0, "expected 0");
    r = parse_pairs("a b=c d", out, 4);
    CHECK("spaces are ordinary characters", r == 1 && strcmp(out[0].key, "a b") == 0 && strcmp(out[0].value, "c d") == 0, "returned %d", r);
    CHECK("a key with no = is malformed", parse_pairs("abc", out, 4) == -1, "expected -1");
    CHECK("a leftover key after a pair is malformed", parse_pairs("a=1;b", out, 4) == -1, "expected -1");
    CHECK("a ; before any = is malformed", parse_pairs("a;b=1", out, 4) == -1, "expected -1");
    CHECK("an empty key is malformed", parse_pairs("=1", out, 4) == -1, "expected -1");
    CHECK("a lone semicolon (empty key) is malformed", parse_pairs(";", out, 4) == -1, "expected -1");
    CHECK("two semicolons in a row (empty key) is malformed", parse_pairs("a=1;;b=2", out, 4) == -1, "expected -1");
    CHECK("an = inside a value is malformed", parse_pairs("a=b=c", out, 4) == -1, "expected -1");
    r = parse_pairs("123456789012345=123456789012345", out, 4);
    CHECK("a 15-character key and value just fit", r == 1 && strlen(out[0].key) == 15 && strlen(out[0].value) == 15, "returned %d", r);
    CHECK("a 16-character key is rejected", parse_pairs("1234567890123456=v", out, 4) == -1, "expected -1");
    CHECK("a 16-character value is rejected", parse_pairs("k=1234567890123456", out, 4) == -1, "expected -1");
    r = parse_pairs("a=1;b=2;c=3;d=4;", out, 4);
    CHECK("exactly max pairs is fine", r == 4 && strcmp(out[3].key, "d") == 0, "returned %d", r);
    CHECK("more than max pairs is malformed", parse_pairs("a=1;b=2;c=3;d=4;e=5", out, 4) == -1, "expected -1");
    CHECK("max of 0 rejects any pair", parse_pairs("a=1", out, 0) == -1, "expected -1");
    CHECK("max of 0 accepts empty text", parse_pairs("", out, 0) == 0, "expected 0");
    return 0;
}
