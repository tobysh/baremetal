#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int *make_range(int n) {
    return NULL;
}

char *duplicate(const char *s) {
    return NULL;
}

// Optional: try them with Run.
int main(void) {
    int *r = make_range(5);
    if (r != NULL) {
        for (int i = 0; i < 5; i++) printf("%d ", r[i]);
        printf("\n");
        free(r);
    }
    char *d = duplicate("hello");
    if (d != NULL) {
        printf("%s\n", d);
        free(d);
    }
    return 0;
}
