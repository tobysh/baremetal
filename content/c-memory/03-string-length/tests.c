int main(void) {
    char embedded[] = {'a', 'b', '\0', 'c', 'd', '\0'};
    char longer[201];
    memset(longer, 'x', 200);
    longer[200] = '\0';

    CHECK("my_strlen(\"\") is 0", my_strlen("") == 0, "got %zu", my_strlen(""));
    CHECK("my_strlen(\"a\") is 1", my_strlen("a") == 1, "got %zu", my_strlen("a"));
    CHECK("my_strlen(\"hello\") is 5", my_strlen("hello") == 5, "got %zu", my_strlen("hello"));
    CHECK("my_strlen(\"hello world\") is 11 (spaces count)", my_strlen("hello world") == 11, "got %zu", my_strlen("hello world"));
    CHECK("my_strlen(\"  \") is 2 (only spaces)", my_strlen("  ") == 2, "got %zu", my_strlen("  "));
    CHECK("my_strlen(\"line\\n\") is 5 (newline counts)", my_strlen("line\n") == 5, "got %zu", my_strlen("line\n"));
    CHECK("my_strlen stops at the first '\\0' (\"ab\\0cd\" gives 2)", my_strlen(embedded) == 2, "got %zu", my_strlen(embedded));
    CHECK("my_strlen of 200 characters is 200", my_strlen(longer) == 200, "got %zu", my_strlen(longer));
    CHECK("my_strlen(embedded + 3) is 2 (starts mid-array)", my_strlen(embedded + 3) == 2, "got %zu", my_strlen(embedded + 3));
    return 0;
}
