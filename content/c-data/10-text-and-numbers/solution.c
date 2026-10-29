#include <stdio.h>

int parse_int(char s[]) {
    int i = 0;
    int sign = 1;
    if (s[i] == '-') {
        sign = -1;
        i++;
    } else if (s[i] == '+') {
        i++;
    }
    int value = 0;
    while (s[i] >= '0' && s[i] <= '9') {
        value = value * 10 + (s[i] - '0');
        i++;
    }
    return sign * value;
}

void int_to_text(int n, char out[]) {
    int len = 0;
    int negative = n < 0;
    if (negative) n = -n;
    if (n == 0) {
        out[len++] = '0';
    }
    while (n > 0) {
        out[len++] = (char)('0' + n % 10);
        n /= 10;
    }
    if (negative) out[len++] = '-';
    out[len] = '\0';
    int left = 0;
    int right = len - 1;
    while (left < right) {
        char tmp = out[left];
        out[left] = out[right];
        out[right] = tmp;
        left++;
        right--;
    }
}

int main(void) {
    printf("%d\n", parse_int("472"));
    printf("%d\n", parse_int("-35 apples"));
    char buf[16];
    int_to_text(-905, buf);
    printf("%s\n", buf);
    return 0;
}
