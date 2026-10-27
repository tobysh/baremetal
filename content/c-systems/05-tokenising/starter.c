#include <stdio.h>

int split_words(const char *s, char words[][16], int max_words) {
    return 0;
}

int parse_int(const char *s, int *out) {
    return 0;
}

// Optional: try them with Run.
int main(void) {
    char words[8][16];
    int n = split_words("  go north  quickly ", words, 8);
    printf("%d words\n", n);
    for (int i = 0; i < n; i++) printf("[%s]\n", words[i]);
    int v = 0;
    if (parse_int("-42", &v)) printf("parsed %d\n", v);
    return 0;
}
