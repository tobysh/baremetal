#include <stdio.h>
#include <string.h>

// Copy src into dst, which has room for cap bytes.
void copy_label(char *dst, size_t cap, const char *src) {
    if (dst == NULL || cap == 0) {
        return;
    }
    if (src == NULL) {
        src = "";
    }
    size_t i;
    for (i = 0; i < cap - 1 && src[i] != '\0'; i++) {
        dst[i] = src[i];
    }
    dst[i] = '\0';
}

// Optional: try it with Run.
int main(void) {
    char buf[8];
    copy_label(buf, sizeof buf, "hello, world");
    printf("%s\n", buf);
    return 0;
}
