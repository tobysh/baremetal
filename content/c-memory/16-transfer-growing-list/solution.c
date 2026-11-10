#include <stdio.h>
#include <stdlib.h>

int main(void) {
    int *data = NULL;
    size_t count = 0, cap = 0;
    long long sum = 0;
    int x;
    while (scanf("%d", &x) == 1) {
        if (count == cap) {
            size_t bigger = cap ? cap * 2 : 4;
            int *grown = realloc(data, bigger * sizeof *data);
            if (!grown) {
                free(data);
                return 1;
            }
            data = grown;
            cap = bigger;
        }
        data[count++] = x;
        sum += x;
    }
    for (size_t i = count; i > 0; i--) printf("%d\n", data[i - 1]);
    size_t above = 0;
    for (size_t i = 0; i < count; i++)
        if ((long long)data[i] * (long long)count > sum) above++;
    printf("above average: %zu\n", above);
    free(data);
    return 0;
}
