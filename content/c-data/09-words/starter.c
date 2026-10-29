#include <stdio.h>

int count_words(char s[]) {
    return -1;
}

int longest_word(char s[]) {
    return -1;
}

void caesar(char s[], int shift) {
}

// Optional: try them with Run.
int main(void) {
    char text[] = "  the quick  brown fox ";
    printf("words = %d (expected 4)\n", count_words(text));
    printf("longest = %d (expected 5)\n", longest_word(text));
    char secret[] = "Hello, Zoe!";
    caesar(secret, 3);
    printf("%s (expected Khoor, Crh!)\n", secret);
    return 0;
}
