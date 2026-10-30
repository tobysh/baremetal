#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void skip_spaces(const char **p) {
    while (**p == ' ') {
        (*p)++;
    }
}

int make_greeting(const char *name, char **out) {
    size_t size = strlen("Hello, ") + strlen(name) + 2;
    char *buf = malloc(size);
    if (buf == NULL) {
        *out = NULL;
        return -1;
    }
    snprintf(buf, size, "Hello, %s!", name);
    *out = buf;
    return 0;
}

int take_word(const char **p, char **out) {
    skip_spaces(p);
    const char *start = *p;
    while (**p != '\0' && **p != ' ') {
        (*p)++;
    }
    size_t len = (size_t)(*p - start);
    if (len == 0) {
        *out = NULL;
        return -1;
    }
    char *word = malloc(len + 1);
    if (word == NULL) {
        *out = NULL;
        return -1;
    }
    memcpy(word, start, len);
    word[len] = '\0';
    *out = word;
    return 0;
}

int main(void) {
    const char *text = "   hello world";
    skip_spaces(&text);
    printf("after skip: \"%s\"\n", text);

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
