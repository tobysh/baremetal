`square_worker` just converts the pointer and writes: `Job *job = arg; job->output = job->input * job->input; return NULL;`.
===
In `squares`, keep two arrays of size 8: `Job jobs[8]` and `pthread_t threads[8]`. A loop fills `jobs[i].input = in[i]` and calls `pthread_create(&threads[i], NULL, square_worker, &jobs[i])`.
===
```c
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
```
