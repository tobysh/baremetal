#include <stdio.h>
#include <string.h>
#include <ctype.h>

size_t count_char(const char *s, char c) {
    return 0;
}

void reverse_prefix(char *s, size_t n) {
}

int is_phrase_palindrome(const char *s) {
    return 0;
}

// Optional: try them with Run.
int main(void) {
    printf("count 'l' in hello: %zu (expected 2)\n", count_char("hello", 'l'));
    char buf[] = "abcdef";
    reverse_prefix(buf, 3);
    printf("reverse_prefix: %s (expected cbadef)\n", buf);
    printf("phrase: %d (expected 1)\n", is_phrase_palindrome("A man, a plan, a canal: Panama"));
    return 0;
}
