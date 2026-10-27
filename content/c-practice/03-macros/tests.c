int main(void) {
    CHECK("SQUARE(4) is 16", SQUARE(4) == 16, "got %d", SQUARE(4));
    CHECK("SQUARE(1 + 2) is 9 (brackets around the argument)", SQUARE(1 + 2) == 9, "got %d, expected 9", SQUARE(1 + 2));
    CHECK("100 / SQUARE(5) is 4 (brackets around the whole macro)", 100 / SQUARE(5) == 4, "got %d, expected 4", 100 / SQUARE(5));
    CHECK("SQUARE(-3) is 9", SQUARE(-3) == 9, "got %d", SQUARE(-3));

    CHECK("MAX(2, 7) is 7", MAX(2, 7) == 7, "got %d", MAX(2, 7));
    CHECK("MAX(5, 2) + 10 is 15", MAX(5, 2) + 10 == 15, "got %d, expected 15", MAX(5, 2) + 10);
    int x = 3, y = 4;
    CHECK("MAX works on variables and expressions", MAX(x + 2, y) == 5, "got %d, expected 5", MAX(x + 2, y));
    CHECK("MAX of equal numbers", MAX(6, 6) == 6, "got %d", MAX(6, 6));
    CHECK("MAX can be compared inside a bigger test", (MAX(1, 2) == 2) && (MAX(3, 1) == 3), "bad comparison");

    int arr[7];
    double ds[3];
    (void)arr; (void)ds;
    CHECK("ARRAY_LEN of int[7] is 7", ARRAY_LEN(arr) == 7, "got %d, expected 7", (int)ARRAY_LEN(arr));
    CHECK("ARRAY_LEN of double[3] is 3", ARRAY_LEN(ds) == 3, "got %d, expected 3", (int)ARRAY_LEN(ds));
    CHECK("ARRAY_LEN works on a string array", ARRAY_LEN("hello") == 6, "got %d, expected 6", (int)ARRAY_LEN("hello"));

    int p = 1, q = 2;
    SWAP_INT(p, q);
    CHECK("SWAP_INT swaps two variables", p == 2 && q == 1, "p = %d, q = %d", p, q);
    int t = 10, u = 20;
    SWAP_INT(t, u);
    CHECK("SWAP_INT works when a variable is named t", t == 20 && u == 10, "t = %d, u = %d, expected 20 and 10", t, u);
    int m = 1, n = 2;
    if (m < n) SWAP_INT(m, n); else m = 99;
    CHECK("SWAP_INT is a single statement (works in if/else)", m == 2 && n == 1, "m = %d, n = %d", m, n);

    CHECK("STRINGIFY(hello) is \"hello\"", strcmp(STRINGIFY(hello), "hello") == 0, "got \"%s\"", STRINGIFY(hello));
    CHECK("STRINGIFY keeps the text you wrote", strcmp(STRINGIFY(a + b), "a + b") == 0, "got \"%s\"", STRINGIFY(a + b));
    return 0;
}
