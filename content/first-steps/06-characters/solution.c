#include <stdio.h>

int main(void) {
    char letter = 'h';

    printf("letter: %c\n", letter);
    printf("code: %d\n", letter);
    printf("next: %c\n", letter + 1);
    printf("upper: %c\n", letter - 'a' + 'A');
    printf("word: %s\n", "hello");
    return 0;
}
