#include <stddef.h>

size_t my_strlen(const char *s);

static void length_case(const char *name, int seeded, const char *s, size_t shown) {
    if (seeded) BM_SEEDED(name); else BM_CASE(name);
    BM_IN_BYTES("s (including the terminator and any trailing bytes)", s, shown);
    BM_OUT_U("my_strlen(s)", my_strlen(s));
}

BM_DRIVER {
    char embedded[] = {'a', 'b', '\0', 'c', 'd', '\0'};
    char longer[201];
    for (int i = 0; i < 200; i++) longer[i] = 'x';
    longer[200] = '\0';

    length_case("my_strlen of an empty string is 0", 0, "", 1);
    length_case("my_strlen of one character is 1", 0, "a", 2);
    length_case("my_strlen of hello is 5", 0, "hello", 6);
    length_case("my_strlen counts spaces", 0, "hello world", 12);
    length_case("my_strlen of two spaces is 2", 0, "  ", 3);
    length_case("my_strlen counts a newline", 0, "line\n", 6);
    length_case("my_strlen stops at the first null byte", 0, embedded, sizeof embedded);
    length_case("my_strlen of 200 characters is 200", 0, longer, sizeof longer);
    length_case("my_strlen can start inside an array", 0, embedded + 3, 3);

    static const char *const names[] = {
        "my_strlen of a fresh string (1)", "my_strlen of a fresh string (2)",
        "my_strlen of a fresh string (3)", "my_strlen of a fresh string (4)",
        "my_strlen of a fresh string (5)", "my_strlen of a fresh string (6)",
    };
    for (int k = 0; k < 6; k++) {
        char text[257];
        size_t n = (size_t)bm_int(1, 240);
        for (size_t i = 0; i < n; i++) text[i] = (char)bm_int('a', 'z');
        text[n] = '\0';
        length_case(names[k], 1, text, n + 1);
    }

    char text[129];
    for (size_t i = 0; i < sizeof text - 1; i++) text[i] = (char)bm_int('a', 'z');
    size_t stop = (size_t)bm_int(1, 60);
    text[stop] = '\0';
    text[sizeof text - 1] = '\0';
    length_case("my_strlen ignores fresh bytes after the first terminator", 1, text, sizeof text);

    char suffix[81];
    for (size_t i = 0; i < sizeof suffix - 1; i++) suffix[i] = (char)bm_int('a', 'z');
    suffix[sizeof suffix - 1] = '\0';
    size_t offset = (size_t)bm_int(1, 79);
    length_case("my_strlen of a fresh suffix", 1, suffix + offset, sizeof suffix - offset);
}
