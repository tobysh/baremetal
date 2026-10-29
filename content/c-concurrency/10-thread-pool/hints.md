`pool_submit`: `Task *t = malloc(sizeof *t); t->fn = fn; t->arg = arg; t->next = NULL;` then under the lock append at `tail` (if `tail` is NULL set `head` too), `p->pending++`, `pthread_cond_signal(&p->has_work)`.
===
The worker waits with `while (p->head == NULL && !p->shutdown) pthread_cond_wait(&p->has_work, &p->lock);`. If `p->head` is still NULL afterwards, shutdown was requested: unlock and return `NULL`. Otherwise unlink the head, set `tail = NULL` if the list became empty, **unlock**, run `t->fn(t->arg)`, `free(t)`, lock again, `pending--`, and `broadcast` on `all_done` when it reaches 0.
===
`pool_wait`: lock, `while (p->pending > 0) pthread_cond_wait(&p->all_done, &p->lock);`, unlock. `pool_destroy`: lock, `shutdown = 1`, broadcast `has_work`, unlock, join every thread, destroy the mutex and both conditions, `free(p)`.
