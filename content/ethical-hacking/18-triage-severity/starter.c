#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

typedef struct { char id[16]; int impact, exploitability, exposure, asset_value, exploited, mitigated; } Finding;

int score_finding(const Finding *f) {
    (void)f;
    return 0;
}

const char *severity_name(int score) {
    (void)score;
    return "info";
}

void triage(Finding *list, int n) {
    (void)list; (void)n;
}

int main(void) {
    Finding f = { "F-1", 3, 2, 2, 3, 1, 0 };
    printf("%d %s\n", score_finding(&f), severity_name(score_finding(&f)));
    return 0;
}
