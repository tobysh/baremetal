#include <stdio.h>

int str_equal(char a[], char b[]) {
    int i = 0;
    while (a[i] != '\0' && a[i] == b[i]) {
        i++;
    }
    return a[i] == b[i];
}

int str_compare(char a[], char b[]) {
    int i = 0;
    while (a[i] != '\0' && a[i] == b[i]) {
        i++;
    }
    return (int)(unsigned char)a[i] - (int)(unsigned char)b[i];
}

void str_copy(char dst[], char src[]) {
    int i = 0;
    while (src[i] != '\0') {
        dst[i] = src[i];
        i++;
    }
    dst[i] = '\0';
}

void str_append(char dst[], char src[]) {
    int end = 0;
    while (dst[end] != '\0') {
        end++;
    }
    int i = 0;
    while (src[i] != '\0') {
        dst[end + i] = src[i];
        i++;
    }
    dst[end + i] = '\0';
}

int main(void) {
    char buf[32];
    str_copy(buf, "tea");
    str_append(buf, "spoon");
    printf("%s\n", buf);
    printf("equal = %d\n", str_equal("cat", "car"));
    printf("compare < 0 : %d\n", str_compare("cat", "car") < 0);
    return 0;
}
