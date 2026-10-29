#include <stdio.h>

int parse_int(char s[]) {
    return -1;
}

void int_to_text(int n, char out[]) {
    out[0] = '?';
    out[1] = '\0';
}

// Optional: try them with Run.
int main(void) {
    printf("%d (expected 472)\n", parse_int("472"));
    printf("%d (expected -35)\n", parse_int("-35 apples"));
    char buf[16];
    int_to_text(-905, buf);
    printf("%s (expected -905)\n", buf);
    return 0;
}
