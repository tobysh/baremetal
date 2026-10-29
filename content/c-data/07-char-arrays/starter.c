#include <stdio.h>
#include <ctype.h>

int str_length(char s[]) {
    return -1;
}

int count_vowels(char s[]) {
    return -1;
}

void to_upper(char s[]) {
}

// Optional: try them with Run.
int main(void) {
    char word[] = "Hello, World";
    printf("length = %d (expected 12)\n", str_length(word));
    printf("vowels = %d (expected 3)\n", count_vowels(word));
    to_upper(word);
    printf("%s (expected HELLO, WORLD)\n", word);
    return 0;
}
