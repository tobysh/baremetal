int main(void) {
    int *r1 = make_range(1);
    CHECK("make_range(1) returns {0}", r1 != NULL && r1[0] == 0, "r1 = %p", (void *)r1);
    free(r1);

    int *r5 = make_range(5);
    int ok5 = r5 != NULL;
    for (int i = 0; ok5 && i < 5; i++) if (r5[i] != i) ok5 = 0;
    CHECK("make_range(5) returns {0, 1, 2, 3, 4}", ok5, "wrong contents or NULL");
    free(r5);

    int *r1000 = make_range(1000);
    int ok = r1000 != NULL;
    for (int i = 0; ok && i < 1000; i++) if (r1000[i] != i) ok = 0;
    CHECK("make_range(1000) fills every element with its index", ok, "wrong contents or NULL");
    free(r1000);

    int *a = make_range(3);
    int *b = make_range(3);
    CHECK("make_range returns a fresh block each call", a != NULL && b != NULL && a != b, "same pointer returned twice");
    if (a != NULL && b != NULL) {
        a[0] = 99;
        CHECK("changing one make_range result does not affect another", b[0] == 0, "b[0] was %d", b[0]);
    }
    free(a);
    free(b);

    char src[] = "hello";
    char *d = duplicate(src);
    CHECK("duplicate(\"hello\") equals \"hello\"", d != NULL && strcmp(d, "hello") == 0, "got \"%s\"", d ? d : "(null)");
    CHECK("duplicate returns a different address than the original", d != NULL && d != src, "same pointer returned");
    src[0] = 'J';
    CHECK("changing the original does not change the copy", d != NULL && strcmp(d, "hello") == 0, "got \"%s\"", d ? d : "(null)");
    free(d);

    char *e = duplicate("");
    CHECK("duplicate(\"\") is an empty string (needs 1 byte for the terminator)", e != NULL && e[0] == '\0', "got %s", e ? "non-empty" : "(null)");
    free(e);

    char *s = duplicate("Hello, World! 123");
    CHECK("duplicate keeps spaces and punctuation", s != NULL && strcmp(s, "Hello, World! 123") == 0, "got \"%s\"", s ? s : "(null)");
    free(s);

    char longer[301];
    memset(longer, 'q', 300);
    longer[300] = '\0';
    char *l = duplicate(longer);
    CHECK("duplicate copies a 300-character string", l != NULL && strlen(l) == 300 && strcmp(l, longer) == 0, "bad copy");
    free(l);
    return 0;
}
