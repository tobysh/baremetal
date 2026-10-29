#define _POSIX_C_SOURCE 200809L
#include <pthread.h>
#include <stdio.h>

typedef struct {
    int input;
    int output;
} Job;

void *square_worker(void *arg) {
    Job *job = arg;
    job->output = job->input * job->input;
    return NULL;
}

void squares(const int *in, int *out, int n) {
    Job jobs[8];
    pthread_t threads[8];
    for (int i = 0; i < n; i++) {
        jobs[i].input = in[i];
        jobs[i].output = 0;
        pthread_create(&threads[i], NULL, square_worker, &jobs[i]);
    }
    for (int i = 0; i < n; i++) {
        pthread_join(threads[i], NULL);
        out[i] = jobs[i].output;
    }
}

int main(void) {
    int in[4] = {1, 2, 3, 4};
    int out[4];
    squares(in, out, 4);
    for (int i = 0; i < 4; i++) printf("%d squared is %d\n", in[i], out[i]);
    return 0;
}
