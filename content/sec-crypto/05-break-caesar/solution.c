#include <stdio.h>
#include <string.h>

static char unshift(char c, int s) {
    if (c >= 'a' && c <= 'z') return (char)('a' + (c - 'a' + 26 - s) % 26);
    if (c >= 'A' && c <= 'Z') return (char)('A' + (c - 'A' + 26 - s) % 26);
    return c;
}

static int score(const char *line, int s) {
    int total = 0;
    for (size_t i = 0; line[i]; i++) {
        char p = unshift(line[i], s);
        if (p >= 'A' && p <= 'Z') p = (char)(p + 32);
        if (p >= 'a' && p <= 'z' && strchr("etaoinshr", p)) total++;
    }
    return total;
}

int main(void) {
    char line[1024];
    if (!fgets(line, sizeof line, stdin)) return 1;
    line[strcspn(line, "\n")] = '\0';

    int best = 1, best_score = -1;
    for (int s = 1; s <= 25; s++) {
        int sc = score(line, s);
        if (sc > best_score) { best_score = sc; best = s; }
    }
    printf("shift: %d\n", best);
    for (size_t i = 0; line[i]; i++) putchar(unshift(line[i], best));
    putchar('\n');
    return 0;
}
