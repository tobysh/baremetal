#include <stdio.h>
#include <stddef.h>

size_t my_strlen(const char *s) {
    size_t n = 0;
    while (s[n] != '\0') {
        n++;
    }
    return n;
}

int main(void) {
    printf("%zu\n", my_strlen("hello"));
    return 0;
}
