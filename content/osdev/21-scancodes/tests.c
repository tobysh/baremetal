static int feed_all(KbdState *s, const uint8_t *seq, int n, char *out) {
    int k = 0;
    for (int i = 0; i < n; i++) {
        int c = kbd_feed(s, seq[i]);
        if (c) out[k++] = (char)c;
    }
    out[k] = 0;
    return k;
}
static int eq(const char *a, const char *b) { while (*a && *a == *b) { a++; b++; } return *a == *b; }
#define SEQ(...) (const uint8_t[]){__VA_ARGS__}, (int)sizeof((const uint8_t[]){__VA_ARGS__})

static void drain_kbd(void) { for (int i = 0; i < 20 && (bm_inb(0x64) & 1); i++) bm_inb(0x60); }

void kmain(void) {
    KbdState s;
    char out[64];

    memset(&s, 0, sizeof s);
    feed_all(&s, SEQ(0x1E, 0x30, 0x2E), out);
    CHECK("make codes for a, b, c give letters", eq(out, "abc"), "got \"%s\"", out);

    memset(&s, 0, sizeof s);
    feed_all(&s, SEQ(0x1E, 0x9E, 0x30, 0xB0), out);
    CHECK("break codes produce nothing", eq(out, "ab"), "got \"%s\"", out);

    memset(&s, 0, sizeof s);
    feed_all(&s, SEQ(0x02, 0x0B, 0x0C, 0x0D, 0x39, 0x1C, 0x0E, 0x0F, 0x01), out);
    CHECK("digits, punctuation and special keys", eq(out, "10-= \n\b\t\033"), "got \"%s\"", out);

    memset(&s, 0, sizeof s);
    feed_all(&s, SEQ(0x35, 0x34, 0x33, 0x27, 0x28, 0x29, 0x2B, 0x1A, 0x1B), out);
    CHECK("punctuation keys", eq(out, "/.,;'`\\[]"), "got \"%s\"", out);

    memset(&s, 0, sizeof s);
    feed_all(&s, SEQ(0x2A, 0x1E, 0x9E, 0xAA, 0x1E), out);
    CHECK("left shift makes a capital letter, and releasing it goes back", eq(out, "Aa"), "got \"%s\"", out);

    memset(&s, 0, sizeof s);
    feed_all(&s, SEQ(0x36, 0x02, 0x03, 0xB6, 0x02), out);
    CHECK("right shift gives the shifted symbols", eq(out, "!@1"), "got \"%s\"", out);

    memset(&s, 0, sizeof s);
    feed_all(&s, SEQ(0x2A, 0x36, 0xAA, 0x1E, 0xB6, 0x1E), out);
    CHECK("shift stays on until both shift keys are released", eq(out, "Aa"), "got \"%s\"", out);

    memset(&s, 0, sizeof s);
    feed_all(&s, SEQ(0x3A, 0x1E, 0x02, 0x3A, 0x1E), out);
    CHECK("caps lock toggles letters but not digits", eq(out, "A1a"), "got \"%s\"", out);

    memset(&s, 0, sizeof s);
    feed_all(&s, SEQ(0x3A, 0xBA, 0x2A, 0x1E, 0xAA, 0x1E), out);
    CHECK("the release of caps lock changes nothing, and shift reverses caps", eq(out, "aA"), "got \"%s\"", out);

    memset(&s, 0, sizeof s);
    feed_all(&s, SEQ(0x1D, 0x1E, 0x2E, 0x16, 0x9D, 0x1E), out);
    CHECK("ctrl+letter gives control codes", out[0] == 1 && out[1] == 3 && out[2] == 21 && out[3] == 'a' && out[4] == 0, "got %d %d %d %d", out[0], out[1], out[2], out[3]);

    memset(&s, 0, sizeof s);
    feed_all(&s, SEQ(0x3A, 0x2A, 0x1D, 0x16), out);
    CHECK("ctrl+u ignores shift and caps lock", out[0] == 21 && out[1] == 0, "got %d", out[0]);

    memset(&s, 0, sizeof s);
    int r = kbd_feed(&s, 0xE0);
    int u = kbd_feed(&s, 0x48);
    CHECK("0xE0 returns 0 and the following 0x48 is the up arrow", r == 0 && u == KEY_UP, "got %d and %d", r, u);
    CHECK("the arrow keys are all different values above 255", ({ memset(&s, 0, sizeof s); int d, l, rr; kbd_feed(&s, 0xE0); d = kbd_feed(&s, 0x50); kbd_feed(&s, 0xE0); l = kbd_feed(&s, 0x4B); kbd_feed(&s, 0xE0); rr = kbd_feed(&s, 0x4D); d == KEY_DOWN && l == KEY_LEFT && rr == KEY_RIGHT && KEY_UP > 255; }), "an arrow key returned the wrong value");
    memset(&s, 0, sizeof s);
    kbd_feed(&s, 0xE0);
    kbd_feed(&s, 0x48);
    int plain = kbd_feed(&s, 0x48);
    CHECK("the 0xE0 flag only applies to the next byte (plain 0x48 is keypad 8, no character here)", plain == 0 && s.e0 == 0, "got %d, e0 is %d", plain, s.e0);
    memset(&s, 0, sizeof s);
    kbd_feed(&s, 0xE0);
    int brk = kbd_feed(&s, 0xC8);
    CHECK("releasing an arrow key produces nothing", brk == 0 && s.e0 == 0, "got %d", brk);
    memset(&s, 0, sizeof s);
    feed_all(&s, SEQ(0xE0, 0x1D, 0x2E, 0xE0, 0x9D, 0x2E), out);
    CHECK("right ctrl (0xE0 0x1D) works like left ctrl", out[0] == 3 && out[1] == 'c' && out[2] == 0, "got %d %d", out[0], out[1]);

    memset(&s, 0, sizeof s);
    feed_all(&s, SEQ(0x38, 0x3B, 0x3A, 0xFF, 0x3C, 0x58), out);
    CHECK("alt, function keys and unknown codes produce nothing", out[0] == 0, "got \"%s\"", out);
    memset(&s, 0, sizeof s);
    CHECK("the highest table entry is the space bar, 0x3A is not a character", kbd_feed(&s, 0x39) == ' ' && kbd_feed(&s, 0x3A) == 0 && kbd_feed(&s, 0x3B) == 0, "table bounds are wrong");

    memset(&s, 0, sizeof s);
    feed_all(&s, SEQ(0x2A, 0x23, 0xA3, 0xAA, 0x17, 0x39, 0x11, 0x18, 0x13, 0x26, 0x20, 0x2A, 0x02, 0xAA), out);
    CHECK("a whole sentence: Hi world!", eq(out, "Hi world!"), "got \"%s\"", out);

    // The real controller. Nothing has been typed, so nothing is waiting.
    drain_kbd();
    CHECK("kbd_poll returns -1 when no byte is waiting", kbd_poll() == -1, "returned %d", kbd_poll());
    // Controller command 0xD2 loads the output buffer as if the keyboard had sent the next data byte.
    bm_outb(0x64, 0xD2);
    bm_outb(0x60, 0x1E);
    for (int i = 0; i < 1000 && !(bm_inb(0x64) & 1); i++) __asm__ volatile("pause");
    int got = kbd_poll();
    CHECK("kbd_poll returns a waiting scancode", got == 0x1E, "returned %d", got);
    CHECK("kbd_poll consumes the byte", kbd_poll() == -1, "a second poll returned %d", kbd_poll());
    bm_outb(0x64, 0xD2);
    bm_outb(0x60, 0x9E);
    for (int i = 0; i < 1000 && !(bm_inb(0x64) & 1); i++) __asm__ volatile("pause");
    CHECK("kbd_poll returns break codes unchanged (as a value above 127)", kbd_poll() == 0x9E, "bytes above 127 must not become negative");
    drain_kbd();
}
