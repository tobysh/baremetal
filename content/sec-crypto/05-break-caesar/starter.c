#include <stdio.h>
#include <string.h>

int main(void) {
    char line[1024];
    if (!fgets(line, sizeof line, stdin)) return 1;

    // Try shifts 1..25, keep the one that looks most like English,
    // then print "shift: N" and the decrypted line.

    return 0;
}
