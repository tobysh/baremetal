int main(void) {
    char *a[16];
    char b[128];
    int n;

    n = tokenize("ls -l /tmp", a, 15, b, sizeof b);
    CHECK("plain words", n == 3 && !strcmp(a[0], "ls") && !strcmp(a[1], "-l") && !strcmp(a[2], "/tmp") && a[3] == NULL, "n = %d", n);

    n = tokenize("  \t echo   hi \t ", a, 15, b, sizeof b);
    CHECK("extra separators are ignored", n == 2 && !strcmp(a[0], "echo") && !strcmp(a[1], "hi") && a[2] == NULL, "n = %d", n);

    n = tokenize("", a, 15, b, sizeof b);
    CHECK("empty line has no words", n == 0 && a[0] == NULL, "n = %d", n);

    n = tokenize("grep \"hello world\" f", a, 15, b, sizeof b);
    CHECK("double quotes group words", n == 3 && !strcmp(a[1], "hello world"), "n = %d, a[1] = %s", n, n > 1 ? a[1] : "?");

    n = tokenize("echo 'a  \"b\" \\n $x'", a, 15, b, sizeof b);
    CHECK("single quotes are fully literal", n == 2 && !strcmp(a[1], "a  \"b\" \\n $x"), "n = %d, a[1] = %s", n, n > 1 ? a[1] : "?");

    n = tokenize("echo \"say \\\"hi\\\" \\\\ \\n\"", a, 15, b, sizeof b);
    CHECK("escapes inside double quotes", n == 2 && !strcmp(a[1], "say \"hi\" \\ \\n"), "n = %d, a[1] = %s", n, n > 1 ? a[1] : "?");

    n = tokenize("a\\ b c\\\\d", a, 15, b, sizeof b);
    CHECK("backslash outside quotes", n == 2 && !strcmp(a[0], "a b") && !strcmp(a[1], "c\\d"), "n = %d", n);

    n = tokenize("x ab\"c d\"ef'  g' y", a, 15, b, sizeof b);
    CHECK("adjacent pieces are glued into one word", n == 3 && !strcmp(a[1], "abc def  g") && !strcmp(a[2], "y"), "n = %d, a[1] = %s", n, n > 1 ? a[1] : "?");

    n = tokenize("a \"\" b ''", a, 15, b, sizeof b);
    CHECK("empty quotes make an empty word", n == 4 && !strcmp(a[1], "") && !strcmp(a[2], "b") && !strcmp(a[3], ""), "n = %d", n);

    n = tokenize("echo \"oops", a, 15, b, sizeof b);
    CHECK("unclosed double quote returns -1", n == -1, "n = %d", n);
    n = tokenize("echo 'oops", a, 15, b, sizeof b);
    CHECK("unclosed single quote returns -1", n == -1, "n = %d", n);

    n = tokenize("a b c d", a, 3, b, sizeof b);
    CHECK("too many words returns -2", n == -2, "n = %d", n);
    n = tokenize("a b c", a, 3, b, sizeof b);
    CHECK("exactly max_args words is fine", n == 3 && a[3] == NULL, "n = %d", n);

    n = tokenize("abc def", a, 15, b, 7);
    CHECK("buffer too small returns -2", n == -2, "n = %d", n);
    n = tokenize("abc def", a, 15, b, 8);
    CHECK("buffer that just fits works", n == 2 && !strcmp(a[1], "def"), "n = %d", n);

    char line[] = "keep 'me' \"intact\"";
    char copy[sizeof line];
    strcpy(copy, line);
    tokenize(line, a, 15, b, sizeof b);
    CHECK("the input line is not modified", !strcmp(line, copy), "line = %s", line);
    return 0;
}
