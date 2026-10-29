#define _POSIX_C_SOURCE 200809L
#include <pthread.h>
#include <stdio.h>

typedef struct {
    int input;
    int output;
} Job;

void *square_worker(void *arg) {
    return NULL;
}

void squares(const int *in, int *out, int n) {
}

// Optional: try it with Run.
int main(void) {
    int in[4] = {1, 2, 3, 4};
    int out[4];
    squares(in, out, 4);
    for (int i = 0; i < 4; i++) printf("%d squared is %d\n", in[i], out[i]);
    return 0;
}
