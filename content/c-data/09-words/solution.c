#include <stdio.h>

int count_words(char s[]) {
    int count = 0;
    int in_word = 0;
    for (int i = 0; s[i] != '\0'; i++) {
        if (s[i] == ' ') {
            in_word = 0;
        } else if (!in_word) {
            in_word = 1;
            count++;
        }
    }
    return count;
}

int longest_word(char s[]) {
    int best = 0;
    int current = 0;
    for (int i = 0; s[i] != '\0'; i++) {
        if (s[i] == ' ') {
            current = 0;
        } else {
            current++;
            if (current > best) best = current;
        }
    }
    return best;
}

void caesar(char s[], int shift) {
    int k = ((shift % 26) + 26) % 26;
    for (int i = 0; s[i] != '\0'; i++) {
        if (s[i] >= 'a' && s[i] <= 'z') {
            s[i] = (char)('a' + (s[i] - 'a' + k) % 26);
        } else if (s[i] >= 'A' && s[i] <= 'Z') {
            s[i] = (char)('A' + (s[i] - 'A' + k) % 26);
        }
    }
}

int main(void) {
    char text[] = "  the quick  brown fox ";
    printf("words = %d\n", count_words(text));
    printf("longest = %d\n", longest_word(text));
    char secret[] = "Hello, Zoe!";
    caesar(secret, 3);
    printf("%s\n", secret);
    return 0;
}
