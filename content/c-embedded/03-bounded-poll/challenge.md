**Track challenge: one deadline for two waits.**

Keep `wait_flag` and add

```c
int wait_two(bus_t *b, uint32_t offA, uint32_t maskA, uint32_t offB, uint32_t maskB, uint32_t timeout);
```

It waits for register A to satisfy `maskA`, then for register B to satisfy `maskB`, under **one shared budget of `timeout` ticks measured from entry**. It returns 0 on success, -1 if A timed out and -2 if B timed out. Each wait follows the same rule as `wait_flag` (read first, then check elapsed since the *shared* start). Reads of both registers cost one tick each in the simulator.

Calling `wait_flag` twice is not enough: it gives the second wait a fresh budget, so the whole operation can take nearly twice as long as promised. The starter does exactly that and passes the easy cases.

**What the tests establish:** a table of (A ready tick, B ready tick, timeout) cases, including some where the second wait would succeed with a fresh budget but must fail with the shared one, and a wrapping start value. A caller-visible deadline is a promise about the whole operation, not about each step.
