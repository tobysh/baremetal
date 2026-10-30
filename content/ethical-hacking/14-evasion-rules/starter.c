#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

int detect_v1(const char *sample) {
    (void)sample;
    return 0;
}

int detect_v2(const char *sample) {
    (void)sample;
    return 0;
}

int main(void) {
    printf("%d\n", detect_v1("NET BEACON update.example.test"));
    return 0;
}
