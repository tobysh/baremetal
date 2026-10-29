`sum_worker`: `Range *r = arg; long total = 0; for (...) total += r->a[i]; return (void *)(intptr_t)total;`
===
In `parallel_sum` keep `Range ranges[8]` and `pthread_t threads[8]`. For thread `i`: `lo = i * chunk`, `hi = lo + chunk`, then clamp both to `n` (`if (lo > n) lo = n;`).
===
Join with `void *ret; pthread_join(threads[i], &ret); sum += (long)(intptr_t)ret;`. Start every thread before you join the first one, or nothing runs in parallel.
