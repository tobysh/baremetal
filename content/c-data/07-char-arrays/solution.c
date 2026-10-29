#include <stdio.h>
#include <ctype.h>

int str_length(char s[]) {
    int i = 0;
    while (s[i] != '\0') {
        i++;
    }
    return i;
}

int count_vowels(char s[]) {
    int count = 0;
    for (int i = 0; s[i] != '\0'; i++) {
        char c = (char)tolower((unsigned char)s[i]);
        if (c == 'a' || c == 'e' || c == 'i' || c == 'o' || c == 'u') {
            count++;
        }
    }
    return count;
}

void to_upper(char s[]) {
    for (int i = 0; s[i] != '\0'; i++) {
        if (s[i] >= 'a' && s[i] <= 'z') {
            s[i] = (char)(s[i] - 'a' + 'A');
        }
    }
}

int main(void) {
    char word[] = "Hello, World";
    printf("length = %d\n", str_length(word));
    printf("vowels = %d\n", count_vowels(word));
    to_upper(word);
    printf("%s\n", word);
    return 0;
}
