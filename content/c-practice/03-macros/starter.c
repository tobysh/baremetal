#include <stdio.h>

#define SQUARE(x) x * x

#define MAX(a, b) a > b ? a : b

#define ARRAY_LEN(arr) 0

#define SWAP_INT(a, b) do { int t = (a); (a) = (b); (b) = t; } while (0)

#define STRINGIFY(x) "x"

// Optional: try them with Run.
int main(void) {
    printf("%d\n", SQUARE(1 + 2));
    return 0;
}
