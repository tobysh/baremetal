int main(void) {
    LineBuf lb;
    char out[64];
    lb_init(&lb);
    CHECK("an empty buffer has no line", lb_next(&lb, out, sizeof out) == -1, "expected -1");

    lb_feed(&lb, "PING\n", 5);
    int n = lb_next(&lb, out, sizeof out);
    CHECK("one complete line is returned without its newline", n == 4 && strcmp(out, "PING") == 0, "returned %d, line \"%s\"", n, out);
    CHECK("the buffer is empty afterwards", lb_next(&lb, out, sizeof out) == -1 && lb.len == 0, "len is %zu", lb.len);

    lb_feed(&lb, "HEL", 3);
    CHECK("a partial line is not returned", lb_next(&lb, out, sizeof out) == -1, "expected -1");
    lb_feed(&lb, "LO\r", 3);
    CHECK("still no line without a newline", lb_next(&lb, out, sizeof out) == -1, "expected -1");
    lb_feed(&lb, "\n", 1);
    n = lb_next(&lb, out, sizeof out);
    CHECK("the line is completed by a later feed, and CRLF is stripped", n == 5 && strcmp(out, "HELLO") == 0, "returned %d, line \"%s\"", n, out);

    lb_feed(&lb, "A\r\nBB\nCCC\r\nDD", 13);
    n = lb_next(&lb, out, sizeof out);
    CHECK("first of three lines", n == 1 && strcmp(out, "A") == 0, "returned %d, line \"%s\"", n, out);
    n = lb_next(&lb, out, sizeof out);
    CHECK("second line (plain LF)", n == 2 && strcmp(out, "BB") == 0, "returned %d, line \"%s\"", n, out);
    n = lb_next(&lb, out, sizeof out);
    CHECK("third line", n == 3 && strcmp(out, "CCC") == 0, "returned %d, line \"%s\"", n, out);
    CHECK("the incomplete tail is kept", lb_next(&lb, out, sizeof out) == -1 && lb.len == 2 && memcmp(lb.buf, "DD", 2) == 0, "len is %zu", lb.len);
    lb_feed(&lb, "\n", 1);
    n = lb_next(&lb, out, sizeof out);
    CHECK("the kept tail becomes the next line", n == 2 && strcmp(out, "DD") == 0, "returned %d, line \"%s\"", n, out);

    lb_feed(&lb, "\n\r\nX\n", 5);
    n = lb_next(&lb, out, sizeof out);
    CHECK("an empty line has length 0", n == 0 && out[0] == '\0', "returned %d", n);
    n = lb_next(&lb, out, sizeof out);
    CHECK("a lone CRLF is an empty line too", n == 0 && out[0] == '\0', "returned %d", n);
    n = lb_next(&lb, out, sizeof out);
    CHECK("the line after empty lines is intact", n == 1 && strcmp(out, "X") == 0, "returned %d, line \"%s\"", n, out);

    lb_init(&lb);
    const char *stream = "SET a 1\r\nGET a\r\nQUIT\r\n";
    const char *want[] = {"SET a 1", "GET a", "QUIT"};
    int seen = 0, ok = 1;
    for (size_t i = 0; stream[i]; i++) {
        lb_feed(&lb, stream + i, 1);
        while ((n = lb_next(&lb, out, sizeof out)) >= 0) {
            if (seen >= 3 || strcmp(out, want[seen]) != 0) ok = 0;
            seen++;
        }
    }
    CHECK("feeding one byte at a time yields the same 3 lines", ok && seen == 3, "saw %d lines", seen);

    lb_init(&lb);
    char big[64];
    memset(big, 'x', sizeof big);
    CHECK("exactly 64 bytes fit", lb_feed(&lb, big, 64) == 0 && lb.len == 64, "len is %zu", lb.len);
    CHECK("one more byte is refused", lb_feed(&lb, "y", 1) == -1, "expected -1");
    CHECK("a refused feed changes nothing", lb.len == 64 && lb.buf[63] == 'x', "len is %zu", lb.len);
    lb_init(&lb);
    lb_feed(&lb, big, 60);
    CHECK("a partly-fitting feed is refused as a whole", lb_feed(&lb, "abcdefgh", 8) == -1 && lb.len == 60, "len is %zu", lb.len);

    char verb[16];
    const char *arg = NULL;
    int r = parse_command("get   Name", verb, &arg);
    CHECK("verb is upper-cased", r == 0 && strcmp(verb, "GET") == 0, "returned %d, verb \"%s\"", r, verb);
    CHECK("spaces before the argument are skipped", arg && strcmp(arg, "Name") == 0, "arg is \"%s\"", arg ? arg : "(null)");
    r = parse_command("quit", verb, &arg);
    CHECK("a command with no argument gets an empty arg", r == 0 && strcmp(verb, "QUIT") == 0 && arg && *arg == '\0', "arg is \"%s\"", arg ? arg : "(null)");
    r = parse_command("SET color dark blue", verb, &arg);
    CHECK("the argument is the whole rest of the line", r == 0 && strcmp(arg, "color dark blue") == 0, "arg is \"%s\"", arg ? arg : "(null)");
    r = parse_command("   ", verb, &arg);
    CHECK("an all-space line is rejected", r == -1, "returned %d", r);
    r = parse_command("", verb, &arg);
    CHECK("an empty line is rejected", r == -1, "returned %d", r);
    r = parse_command("abcdefghijklmnop x", verb, &arg);
    CHECK("a 16 character verb is rejected", r == -1, "returned %d", r);
    r = parse_command("abcdefghijklmno x", verb, &arg);
    CHECK("a 15 character verb is accepted", r == 0 && strcmp(verb, "ABCDEFGHIJKLMNO") == 0, "returned %d", r);
    return 0;
}
