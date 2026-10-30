#include <stdio.h>
#include <string.h>

void reverse(char *s) {
    size_t len = strlen(s);
    if (len < 2) return;
    size_t left = 0;
    size_t right = len - 1;
    while (left < right) {
        char tmp = s[left];
        s[left] = s[right];
        s[right] = tmp;
        left++;
        right--;
    }
}

int main(void) {
    char word[] = "hello";
    reverse(word);
    printf("%s\n", word);
    return 0;
}
