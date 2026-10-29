The `Limiter` struct and the `PingPong` struct are in the starter. Implement:

- `void limiter_init(Limiter *l, int limit)`: start a semaphore at `limit`.
- `void limiter_enter(Limiter *l)`: take a slot, waiting if none is free.
- `void limiter_leave(Limiter *l)`: give the slot back.
- `void limiter_destroy(Limiter *l)`.
- `void *ping_worker(void *arg)` and `void *pong_worker(void *arg)`: `arg` is a `PingPong`. Ping appends `'P'` to `log` and pong appends `'p'`, each `rounds` times, taking strict turns (ping first). Use `pp->turn_ping` and `pp->turn_pong` and return `NULL`. Write each character at index `pp->pos` and then increase `pos`.
- `void ping_pong(int rounds, char *log)`: initialise the semaphores and the struct (`pos = 0`), start both threads, join them, and end `log` with `'\0'`. The result for 3 rounds is `"PpPpPp"`. `log` has room for `2 * rounds + 1` characters.
