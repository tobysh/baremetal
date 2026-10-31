Implement `flash_unlock` and `flash_program` for the controller in the lesson. `wait_not_busy` is provided.

- `flash_unlock`: if STATUS shows LOCKED, write the two keys in order, once; if still LOCKED afterwards return -4, else 0. If already unlocked, write nothing to KEY.
- `flash_program(b, addr, data, timeout)`: unlock first (-4 on failure). Then up to 3 attempts of: wait not busy, write ADDR, DATA, CMD=1, wait not busy, read STATUS; on ERR write exactly the ERR bit to STATUS and retry; otherwise return 0.
- Return -3 after 3 failed attempts and -1 as soon as a wait times out (no retry).
- Several programs in a row must unlock only once. The controller may already be busy when you start.

Transient failures, protected addresses, written words, slow controllers and busy-at-start are tried.
