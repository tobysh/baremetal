#include <stdio.h>

int main(void) {
    int x, previous = 0, have_previous = 0;
    int rises = 0, biggest = 0;
    while (scanf("%d", &x) == 1) {
        if (have_previous && x > previous) {
            int size = x - previous;
            rises++;
            if (rises == 1 || size > biggest) biggest = size;
        }
        previous = x;
        have_previous = 1;
    }
    printf("rises: %d\n", rises);
    if (rises == 0) printf("biggest rise: none\n");
    else printf("biggest rise: %d\n", biggest);
    return 0;
}
