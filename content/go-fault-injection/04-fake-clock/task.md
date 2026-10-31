Implement `NewLimiter`, `(*Limiter).Allow` and `(*Limiter).Tokens` following the three refill rules in the lesson.

- `NewLimiter(c Clock, capacity int, intervalMs int64) *Limiter` starts full.
- `Allow() bool` and `Tokens() int` both refill first.
- Tokens never go below 0 or above `capacity`, whatever the clock does.
- A jump of up to `math.MaxInt64/2` milliseconds must not overflow: compare `n` with `int64(capacity-tokens)` instead of adding first.

The tests include: an hour backward and then exactly one interval forward gives exactly one new token, a giant forward leap gives exactly `capacity` tokens, and 300 random clock scripts compared against a reference model.
