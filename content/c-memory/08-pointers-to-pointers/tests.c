int main(void) {
    const char *t1 = "   hello world";
    const char *orig1 = t1;
    skip_spaces(&t1);
    CHECK("skip_spaces moves the caller's pointer past leading spaces", t1 == orig1 + 3 && strcmp(t1, "hello world") == 0, "got \"%s\"", t1);

    const char *t2 = "abc";
    skip_spaces(&t2);
    CHECK("skip_spaces leaves a pointer that is already on a non-space", strcmp(t2, "abc") == 0, "got \"%s\"", t2);

    const char *t3 = "     ";
    skip_spaces(&t3);
    CHECK("skip_spaces stops at the terminator for all-space input", *t3 == '\0', "stopped at '%c'", *t3);

    const char *t4 = "";
    skip_spaces(&t4);
    CHECK("skip_spaces on an empty string stays put", *t4 == '\0', "bad pointer");

    char *g = (char *)0x1;
    int r = make_greeting("Ada", &g);
    CHECK("make_greeting(\"Ada\") returns 0 and sets *out to \"Hello, Ada!\"", r == 0 && g != NULL && g != (char *)0x1 && strcmp(g, "Hello, Ada!") == 0, "r = %d, got \"%s\"", r, (g && g != (char *)0x1) ? g : "(null)");
    if (g != (char *)0x1) free(g);

    char *g2 = NULL;
    r = make_greeting("", &g2);
    CHECK("make_greeting with an empty name gives \"Hello, !\"", r == 0 && g2 != NULL && strcmp(g2, "Hello, !") == 0, "r = %d, got \"%s\"", r, g2 ? g2 : "(null)");
    free(g2);

    char *g3 = NULL;
    char *g4 = NULL;
    make_greeting("Grace Hopper", &g3);
    make_greeting("Linus", &g4);
    CHECK("two greetings are separate heap blocks", g3 != NULL && g4 != NULL && g3 != g4 && strcmp(g3, "Hello, Grace Hopper!") == 0 && strcmp(g4, "Hello, Linus!") == 0, "g3 = \"%s\", g4 = \"%s\"", g3 ? g3 : "(null)", g4 ? g4 : "(null)");
    free(g3);
    free(g4);

    const char *cur = "  one  two three ";
    char *w = NULL;
    r = take_word(&cur, &w);
    CHECK("take_word returns the first word \"one\"", r == 0 && w != NULL && strcmp(w, "one") == 0, "r = %d, got \"%s\"", r, w ? w : "(null)");
    free(w);
    CHECK("take_word leaves the cursor just after the word", strcmp(cur, "  two three ") == 0, "cursor at \"%s\"", cur);

    w = NULL;
    r = take_word(&cur, &w);
    CHECK("the second take_word returns \"two\"", r == 0 && w != NULL && strcmp(w, "two") == 0, "r = %d, got \"%s\"", r, w ? w : "(null)");
    free(w);
    w = NULL;
    r = take_word(&cur, &w);
    CHECK("the third take_word returns \"three\"", r == 0 && w != NULL && strcmp(w, "three") == 0, "r = %d, got \"%s\"", r, w ? w : "(null)");
    free(w);

    w = (char *)0x1;
    r = take_word(&cur, &w);
    CHECK("take_word with only spaces left returns -1 and sets *out to NULL", r == -1 && w == NULL, "r = %d, *out = %p", r, (void *)w);
    CHECK("take_word at the end leaves the cursor on the terminator", *cur == '\0', "cursor at \"%s\"", cur);

    const char *e = "";
    w = (char *)0x1;
    r = take_word(&e, &w);
    CHECK("take_word on an empty string returns -1 with *out NULL", r == -1 && w == NULL, "r = %d", r);

    char src[] = "word";
    const char *sp = src;
    w = NULL;
    take_word(&sp, &w);
    src[0] = 'W';
    CHECK("the word is a copy, not a pointer into the original", w != NULL && strcmp(w, "word") == 0, "got \"%s\"", w ? w : "(null)");
    free(w);
    return 0;
}
