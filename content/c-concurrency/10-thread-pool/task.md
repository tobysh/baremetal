The `Task` and `Pool` structs are in the starter. The pool never has more than 8 workers. Implement:

- `Pool *pool_create(int nthreads)`: allocate and initialise a pool (`calloc` it, initialise the mutex and both condition variables), start `nthreads` threads running `pool_worker`, and return it.
- `void *pool_worker(void *arg)`: the worker loop from the lesson. `arg` is the `Pool`.
- `void pool_submit(Pool *p, TaskFn fn, void *arg)`: allocate a `Task`, append it to the queue, count it in `pending`, and wake one worker.
- `void pool_wait(Pool *p)`: block until `pending == 0`. Return at once if nothing was submitted. The pool must be usable again afterwards.
- `void pool_destroy(Pool *p)`: let the workers finish everything already submitted, stop them, join them, and free all memory.

Tasks may be submitted from several threads at once, so every access to the queue needs the lock.
