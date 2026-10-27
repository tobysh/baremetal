#include <stdio.h>

#define SQUARE(x) ((x) * (x))

#define MAX(a, b) ((a) > (b) ? (a) : (b))

#define ARRAY_LEN(arr) (sizeof(arr) / sizeof((arr)[0]))

#define SWAP_INT(a, b) do { int swap_tmp_ = (a); (a) = (b); (b) = swap_tmp_; } while (0)

#define STRINGIFY(x) #x

int main(void) {
    printf("%d\n", SQUARE(1 + 2));
    return 0;
}
