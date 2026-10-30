#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void skip_spaces(const char **p) {
}

int make_greeting(const char *name, char **out) {
    *out = NULL;
    return -1;
}

int take_word(const char **p, char **out) {
    *out = NULL;
    return -1;
}

// Optional: try them with Run.
int main(void) {
    const char *text = "   hello world";
    skip_spaces(&text);
    printf("after skip: \"%s\" (expected \"hello world\")\n", text);

    char *g;
    if (make_greeting("Ada", &g) == 0) {
        printf("%s\n", g);
        free(g);
    }

    const char *cur = "  one two";
    char *w;
    while (take_word(&cur, &w) == 0) {
        printf("word: %s\n", w);
        free(w);
    }
    return 0;
}
