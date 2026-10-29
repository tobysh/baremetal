`Job` is defined in the starter. Implement:

- `void *square_worker(void *arg)`: `arg` points to a `Job`. Store `input * input` in `output` and return `NULL`.
- `void squares(const int *in, int *out, int n)`: for each `i` in `0..n-1`, start **one thread** running `square_worker` on a job for `in[i]`, and put the result in `out[i]`. Start all the threads first, then join them all. `n` is at most 8 (it can be 0).

Include `#define _POSIX_C_SOURCE 200809L` at the top, as the starter does.
