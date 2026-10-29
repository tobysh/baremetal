int main(void) {
    CHECK("parse_int(\"472\") is 472", parse_int("472") == 472, "got %d", parse_int("472"));
    CHECK("parse_int(\"0\") is 0", parse_int("0") == 0, "got %d", parse_int("0"));
    CHECK("parse_int handles a minus sign", parse_int("-35") == -35, "got %d", parse_int("-35"));
    CHECK("parse_int handles a plus sign", parse_int("+8") == 8, "got %d", parse_int("+8"));
    CHECK("parse_int stops at the first non-digit", parse_int("123abc456") == 123, "got %d", parse_int("123abc456"));
    CHECK("parse_int stops at a space", parse_int("-35 apples") == -35, "got %d", parse_int("-35 apples"));
    CHECK("parse_int of text without digits is 0", parse_int("hello") == 0, "got %d", parse_int("hello"));
    CHECK("parse_int of empty text is 0", parse_int("") == 0, "got %d", parse_int(""));
    CHECK("parse_int of a lone sign is 0", parse_int("-") == 0 && parse_int("+") == 0, "expected 0");
    CHECK("parse_int ignores a second sign", parse_int("--5") == 0, "got %d", parse_int("--5"));
    CHECK("parse_int keeps leading zeros harmless", parse_int("007") == 7, "got %d", parse_int("007"));
    CHECK("parse_int handles a large value", parse_int("2000000000") == 2000000000, "got %d", parse_int("2000000000"));
    CHECK("parse_int handles the smallest negative value in range", parse_int("-2147483647") == -2147483647, "got %d", parse_int("-2147483647"));

    char b[24];
    int_to_text(472, b);
    CHECK("int_to_text(472)", strcmp(b, "472") == 0, "got \"%s\"", b);
    int_to_text(0, b);
    CHECK("int_to_text(0) is \"0\"", strcmp(b, "0") == 0, "got \"%s\"", b);
    int_to_text(-905, b);
    CHECK("int_to_text(-905)", strcmp(b, "-905") == 0, "got \"%s\"", b);
    int_to_text(7, b);
    CHECK("int_to_text of one digit", strcmp(b, "7") == 0, "got \"%s\"", b);
    int_to_text(1000, b);
    CHECK("int_to_text keeps trailing zeros", strcmp(b, "1000") == 0, "got \"%s\"", b);
    int_to_text(2147483647, b);
    CHECK("int_to_text of the largest int", strcmp(b, "2147483647") == 0, "got \"%s\"", b);
    int_to_text(-1, b);
    CHECK("int_to_text(-1)", strcmp(b, "-1") == 0, "got \"%s\"", b);
    char c[24] = "ZZZZZZZZZZZZZZZZZZZZZZZ";
    int_to_text(12, c);
    CHECK("int_to_text writes the terminator over old content", strcmp(c, "12") == 0, "got \"%s\"", c);
    int ok = 1;
    for (int n = -120; n <= 120; n += 7) {
        char t[24];
        int_to_text(n, t);
        if (parse_int(t) != n) ok = 0;
    }
    CHECK("parse_int(int_to_text(n)) round-trips", ok, "some value did not round-trip");
    return 0;
}
