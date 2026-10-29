Implement a `Limiter` that allows at most `n` holders at a time.

- `NewLimiter(n int) *Limiter` (treat `n < 1` as `1`).
- `Acquire()` takes a slot, waiting while all are taken.
- `Release()` frees a slot.
- `TryAcquire() bool` takes a slot only if one is free right now, and reports whether it did. It never waits.
- `AcquireCtx(ctx context.Context) error` waits for a slot like `Acquire`, but gives up when `ctx` is done and returns `ctx.Err()`. A failed call must not take a slot. Return `nil` on success.
- `Do(f func())` acquires a slot, runs `f`, and releases the slot even if `f` panics.
