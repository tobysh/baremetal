`limiter_init` is `sem_init(&l->slots, 0, limit)`. `limiter_enter` is `sem_wait(&l->slots)` and `limiter_leave` is `sem_post(&l->slots)`.
===
Ping's loop: `for (r = 0; r < pp->rounds; r++) { sem_wait(&pp->turn_ping); pp->log[pp->pos++] = 'P'; sem_post(&pp->turn_pong); }`. Pong is the mirror image, waiting on `turn_pong` and posting `turn_ping`.
===
In `ping_pong`, set `turn_ping` to start at 1 and `turn_pong` at 0 with two `sem_init` calls. Only one thread is ever between its wait and post, so `pos` needs no extra lock: the semaphores already order the writes.
