#include <stdio.h>
#include <string.h>
#include <ctype.h>

int is_palindrome(const char *s) {
    size_t len = strlen(s);
    if (len < 2) return 1;
    size_t left = 0;
    size_t right = len - 1;
    while (left < right) {
        if (s[left] != s[right]) {
            return 0;
        }
        left++;
        right--;
    }
    return 1;
}

int is_palindrome_ignoring_case(const char *s) {
    size_t len = strlen(s);
    if (len < 2) return 1;
    size_t left = 0;
    size_t right = len - 1;
    while (left < right) {
        int a = tolower((unsigned char)s[left]);
        int b = tolower((unsigned char)s[right]);
        if (a != b) {
            return 0;
        }
        left++;
        right--;
    }
    return 1;
}

int main(void) {
    printf("racecar: %d\n", is_palindrome("racecar"));
    printf("Level:   %d\n", is_palindrome_ignoring_case("Level"));
    return 0;
}
