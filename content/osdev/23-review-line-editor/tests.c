static size_t slen(const char *s) { size_t n = 0; while (s[n]) n++; return n; }
static int eq(const char *a, const char *b) { while (*a && *a == *b) { a++; b++; } return *a == *b; }

static void loopback(void) {
    serial_init();
    bm_outb(COM1 + 4, 0x13);
    while (bm_inb(COM1 + 5) & 1) bm_inb(COM1);
}
static void echo_of(char *buf, int max) {
    int n = 0;
    while ((bm_inb(COM1 + 5) & 1) && n < max - 1) buf[n++] = (char)bm_inb(COM1);
    buf[n] = 0;
}
static void feed_str(LineEditor *e, const char *s) { for (; *s; s++) line_feed_char(e, (unsigned char)*s); }

void kmain(void) {
    LineEditor e;
    char echo[40];

    memset(&e, 0x55, sizeof e);
    line_init(&e);
    CHECK("line_init gives an empty, unfinished line", eq(line_text(&e), "") && e.len == 0 && e.done == 0, "text \"%s\", len %lu, done %d", line_text(&e), (unsigned long)e.len, e.done);

    loopback();
    feed_str(&e, "ls");
    echo_of(echo, 40);
    CHECK("printable characters are stored", eq(line_text(&e), "ls"), "text is \"%s\"", line_text(&e));
    CHECK("printable characters are echoed", eq(echo, "ls"), "echo was \"%s\"", echo);

    loopback();
    int r = line_feed_char(&e, 'x');
    CHECK("a printable character does not finish the line", r == 0 && e.done == 0, "returned %d", r);
    echo_of(echo, 40);

    loopback();
    line_feed_char(&e, '\b');
    echo_of(echo, 40);
    CHECK("backspace removes the last character", eq(line_text(&e), "ls"), "text is \"%s\"", line_text(&e));
    CHECK("backspace echoes back, space, back", eq(echo, "\b \b"), "echo was %d bytes starting 0x%02x", (int)slen(echo), echo[0]);

    line_init(&e);
    loopback();
    line_feed_char(&e, '\b');
    echo_of(echo, 40);
    CHECK("backspace on an empty line does nothing", e.len == 0 && eq(line_text(&e), ""), "len is %lu", (unsigned long)e.len);
    CHECK("backspace on an empty line echoes nothing", eq(echo, ""), "echo was %d bytes", (int)slen(echo));

    line_init(&e);
    loopback();
    feed_str(&e, "abcde");
    echo_of(echo, 40);
    loopback();
    r = line_feed_char(&e, 21);
    echo_of(echo, 40);
    CHECK("Ctrl+U clears the whole line", r == 0 && eq(line_text(&e), "") && e.len == 0, "text is \"%s\"", line_text(&e));
    CHECK("Ctrl+U echoes one erase per character", eq(echo, "\b \b\b \b\b \b\b \b\b \b"), "echo was %d bytes (expected 15)", (int)slen(echo));

    line_init(&e);
    feed_str(&e, "abc");
    loopback();
    r = line_feed_char(&e, 3);
    echo_of(echo, 40);
    CHECK("Ctrl+C abandons the line and echoes ^C", r == 0 && eq(line_text(&e), "") && eq(echo, "^C\r\n"), "text \"%s\", echo %d bytes", line_text(&e), (int)slen(echo));

    line_init(&e);
    feed_str(&e, "hi");
    loopback();
    r = line_feed_char(&e, '\n');
    echo_of(echo, 40);
    CHECK("Enter finishes the line and returns 1", r == 1 && e.done == 1, "returned %d, done %d", r, e.done);
    CHECK("Enter echoes carriage return and line feed", eq(echo, "\r\n"), "echo was %d bytes", (int)slen(echo));
    CHECK("the text stays available after Enter", eq(line_text(&e), "hi"), "text is \"%s\"", line_text(&e));

    loopback();
    line_feed_char(&e, 'a');
    echo_of(echo, 40);
    CHECK("a character after Enter starts a fresh line", eq(line_text(&e), "a") && e.done == 0, "text is \"%s\", done %d", line_text(&e), e.done);

    line_init(&e);
    loopback();
    feed_str(&e, "0123456789ABCDE");
    echo_of(echo, 40);
    CHECK("a line holds LINE_MAX - 1 characters", eq(line_text(&e), "0123456789ABCDE") && e.len == 15, "text is \"%s\"", line_text(&e));
    loopback();
    feed_str(&e, "XYZ");
    echo_of(echo, 40);
    CHECK("characters beyond the limit are not stored", eq(line_text(&e), "0123456789ABCDE") && e.len == 15, "text is \"%s\"", line_text(&e));
    CHECK("characters beyond the limit are not echoed", eq(echo, ""), "echo was \"%s\"", echo);
    loopback();
    line_feed_char(&e, '\b');
    line_feed_char(&e, 'Q');
    echo_of(echo, 40);
    CHECK("after backspace there is room again", eq(line_text(&e), "0123456789ABCDQ"), "text is \"%s\"", line_text(&e));

    line_init(&e);
    loopback();
    line_feed_char(&e, '\t');
    line_feed_char(&e, 0x101);
    line_feed_char(&e, 1);
    line_feed_char(&e, 200);
    line_feed_char(&e, 127 + 0x100);
    echo_of(echo, 40);
    CHECK("other characters are ignored without echo", e.len == 0 && eq(echo, ""), "len %lu, echo %d bytes", (unsigned long)e.len, (int)slen(echo));

    // From scancodes.
    KbdState k;
    memset(&k, 0, sizeof k);
    line_init(&e);
    loopback();
    r = 0;
    const uint8_t seq[] = {0x2A, 0x23, 0xA3, 0xAA, 0x17, 0x39, 0x1E, 0x1E, 0x0E, 0x30};   // H i space a a Backspace b
    for (unsigned i = 0; i < sizeof seq; i++) r |= line_feed_scancode(&e, &k, seq[i]);
    echo_of(echo, 40);
    CHECK("scancodes are decoded, shift is applied and backspace works", r == 0 && eq(line_text(&e), "Hi ab"), "text is \"%s\"", line_text(&e));
    CHECK("the keystrokes are echoed", eq(echo, "Hi aa\b \bb"), "echo was \"%s\"", echo);

    loopback();
    r = line_feed_scancode(&e, &k, 0xE0);
    r |= line_feed_scancode(&e, &k, 0x48);
    r |= line_feed_scancode(&e, &k, 0x2A);
    r |= line_feed_scancode(&e, &k, 0xAA);
    r |= line_feed_scancode(&e, &k, 0x3A);
    echo_of(echo, 40);
    CHECK("arrow keys, shift and caps lock change nothing", r == 0 && eq(line_text(&e), "Hi ab") && eq(echo, ""), "text \"%s\", echo %d bytes", line_text(&e), (int)slen(echo));

    loopback();
    r = line_feed_scancode(&e, &k, 0x1C);
    echo_of(echo, 40);
    CHECK("the Enter scancode finishes the line", r == 1 && e.done == 1 && eq(echo, "\r\n"), "returned %d", r);

    memset(&k, 0, sizeof k);
    line_init(&e);
    loopback();
    const uint8_t cu[] = {0x1E, 0x30, 0x1D, 0x16, 0x9D};   // a b Ctrl+U
    for (unsigned i = 0; i < sizeof cu; i++) line_feed_scancode(&e, &k, cu[i]);
    echo_of(echo, 40);
    CHECK("Ctrl+U works from raw scancodes", eq(line_text(&e), "") && eq(echo, "ab\b \b\b \b"), "text \"%s\", echo %d bytes", line_text(&e), (int)slen(echo));

    // line_take
    line_init(&e);
    feed_str(&e, "echo");
    line_feed_char(&e, '\n');
    char out[16];
    memset(out, '#', sizeof out);
    size_t n = line_take(&e, out, sizeof out);
    CHECK("line_take copies the line and returns its length", n == 4 && eq(out, "echo"), "returned %lu, text \"%s\"", (unsigned long)n, out);
    CHECK("line_take starts a new empty line", eq(line_text(&e), "") && e.len == 0 && e.done == 0, "text \"%s\", done %d", line_text(&e), e.done);
    feed_str(&e, "abcdef");
    memset(out, '#', sizeof out);
    n = line_take(&e, out, 4);
    CHECK("line_take truncates to cap - 1 characters and reports the full length", n == 6 && eq(out, "abc") && out[4] == '#', "returned %lu, text \"%s\"", (unsigned long)n, out);
    feed_str(&e, "zz");
    memset(out, '#', sizeof out);
    n = line_take(&e, out, 0);
    CHECK("line_take with cap 0 writes nothing", n == 2 && out[0] == '#', "returned %lu, first byte '%c'", (unsigned long)n, out[0]);
}
