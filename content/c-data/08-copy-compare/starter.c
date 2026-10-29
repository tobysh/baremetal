#include <stdio.h>

int str_equal(char a[], char b[]) {
    return -1;
}

int str_compare(char a[], char b[]) {
    return 99;
}

void str_copy(char dst[], char src[]) {
}

void str_append(char dst[], char src[]) {
}

// Optional: try them with Run.
int main(void) {
    char buf[32];
    str_copy(buf, "tea");
    str_append(buf, "spoon");
    printf("%s (expected teaspoon)\n", buf);
    printf("equal = %d (expected 0)\n", str_equal("cat", "car"));
    printf("compare < 0 : %d (expected 0)\n", str_compare("cat", "car") < 0);
    return 0;
}
