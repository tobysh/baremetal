#include <stdio.h>

int split_words(const char *s, char words[][16], int max_words) {
    int count = 0;
    int i = 0;
    while (count < max_words) {
        while (s[i] == ' ') {
            i++;
        }
        if (s[i] == '\0') {
            break;
        }
        int start = i;
        while (s[i] != '\0' && s[i] != ' ') {
            i++;
        }
        int length = i - start;
        if (length > 15) {
            length = 15;
        }
        for (int k = 0; k < length; k++) {
            words[count][k] = s[start + k];
        }
        words[count][length] = '\0';
        count++;
    }
    return count;
}

int parse_int(const char *s, int *out) {
    int i = 0;
    int negative = 0;
    if (s[i] == '-' || s[i] == '+') {
        negative = (s[i] == '-');
        i++;
    }
    if (s[i] == '\0') {
        return 0;
    }
    int value = 0;
    for (; s[i] != '\0'; i++) {
        if (s[i] < '0' || s[i] > '9') {
            return 0;
        }
        value = value * 10 + (s[i] - '0');
    }
    *out = negative ? -value : value;
    return 1;
}

int main(void) {
    char words[8][16];
    int n = split_words("  go north  quickly ", words, 8);
    printf("%d words\n", n);
    for (int i = 0; i < n; i++) printf("[%s]\n", words[i]);
    int v = 0;
    if (parse_int("-42", &v)) printf("parsed %d\n", v);
    return 0;
}
