In the worker: `long local[256] = {0};`, then `local[t->data[i]]++` over the slice. Only after the loop do you lock.
===
After the loop: `pthread_mutex_lock(t->lock); for (int b = 0; b < 256; b++) t->shared[b] += local[b]; pthread_mutex_unlock(t->lock);`
===
In `parallel_histogram`, declare `pthread_mutex_t lock;` and `pthread_mutex_init(&lock, NULL)`, make `Task tasks[8]`, fill each with its clamped `lo` and `hi`, `hist` and `&lock`, create all threads, then join all, then destroy the mutex.
