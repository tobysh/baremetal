#include <stdio.h>
#include <string.h>
#include <ctype.h>

size_t count_char(const char *s, char c) {
    size_t count = 0;
    for (; *s != '\0'; s++) {
        if (*s == c) {
            count++;
        }
    }
    return count;
}

void reverse_prefix(char *s, size_t n) {
    if (n < 2) return;
    size_t left = 0;
    size_t right = n - 1;
    while (left < right) {
        char tmp = s[left];
        s[left] = s[right];
        s[right] = tmp;
        left++;
        right--;
    }
}

int is_phrase_palindrome(const char *s) {
    size_t len = strlen(s);
    if (len == 0) return 1;
    size_t left = 0;
    size_t right = len - 1;
    while (left < right) {
        if (!isalnum((unsigned char)s[left])) {
            left++;
        } else if (!isalnum((unsigned char)s[right])) {
            right--;
        } else {
            if (tolower((unsigned char)s[left]) != tolower((unsigned char)s[right])) {
                return 0;
            }
            left++;
            right--;
        }
    }
    return 1;
}

int main(void) {
    printf("count 'l' in hello: %zu\n", count_char("hello", 'l'));
    char buf[] = "abcdef";
    reverse_prefix(buf, 3);
    printf("reverse_prefix: %s\n", buf);
    printf("phrase: %d\n", is_phrase_palindrome("A man, a plan, a canal: Panama"));
    return 0;
}
