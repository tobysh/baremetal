int main(void) {
    char w[8][16];
    int n = split_words("go north", w, 8);
    CHECK("\"go north\" splits into 2 words", n == 2 && strcmp(w[0], "go") == 0 && strcmp(w[1], "north") == 0, "n = %d", n);

    n = split_words("  take   the  lamp  ", w, 8);
    CHECK("extra spaces at the start, middle and end are ignored", n == 3 && strcmp(w[0], "take") == 0 && strcmp(w[1], "the") == 0 && strcmp(w[2], "lamp") == 0, "n = %d", n);

    n = split_words("", w, 8);
    CHECK("an empty string has no words", n == 0, "n = %d", n);

    n = split_words("     ", w, 8);
    CHECK("only spaces has no words", n == 0, "n = %d", n);

    n = split_words("single", w, 8);
    CHECK("one word with no spaces", n == 1 && strcmp(w[0], "single") == 0, "n = %d", n);

    n = split_words("a b c d e f", w, 3);
    CHECK("stops after max_words", n == 3 && strcmp(w[2], "c") == 0, "n = %d", n);

    n = split_words("abcdefghijklmnopqrstuvwxyz ok", w, 8);
    CHECK("long words are cut to 15 characters", n == 2 && strcmp(w[0], "abcdefghijklmno") == 0 && strcmp(w[1], "ok") == 0, "n = %d, first = \"%s\"", n, w[0]);

    const char *orig = "keep me";
    char copy[16];
    strcpy(copy, orig);
    split_words(copy, w, 8);
    CHECK("the original string is not modified", strcmp(copy, "keep me") == 0, "string became \"%s\"", copy);

    int v = 999;
    CHECK("parse_int(\"472\") succeeds", parse_int("472", &v) == 1 && v == 472, "v = %d", v);
    CHECK("parse_int(\"0\") is zero", parse_int("0", &v) == 1 && v == 0, "v = %d", v);
    CHECK("parse_int(\"-15\") is negative", parse_int("-15", &v) == 1 && v == -15, "v = %d", v);
    CHECK("parse_int(\"+8\") accepts a plus sign", parse_int("+8", &v) == 1 && v == 8, "v = %d", v);
    CHECK("parse_int(\"2000000\") handles larger numbers", parse_int("2000000", &v) == 1 && v == 2000000, "v = %d", v);

    v = 555;
    CHECK("parse_int rejects the empty string", parse_int("", &v) == 0 && v == 555, "v = %d", v);
    CHECK("parse_int rejects a lone sign", parse_int("-", &v) == 0 && v == 555, "v = %d", v);
    CHECK("parse_int rejects trailing letters", parse_int("12a", &v) == 0 && v == 555, "v = %d", v);
    CHECK("parse_int rejects spaces", parse_int(" 12", &v) == 0 && v == 555, "v = %d", v);
    CHECK("parse_int rejects decimals", parse_int("3.5", &v) == 0 && v == 555, "v = %d", v);
    CHECK("parse_int rejects a double sign", parse_int("--3", &v) == 0 && v == 555, "v = %d", v);
    return 0;
}
