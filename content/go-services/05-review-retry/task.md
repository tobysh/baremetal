`Retry(ctx context.Context, attempts int, backoff, perAttempt time.Duration, f func(ctx context.Context) error) error`

- Treat `attempts < 1` as `1`.
- Each attempt calls `f` with a context derived from `ctx` that times out after `perAttempt`. If `perAttempt <= 0` there is no extra timeout. Release that context after each attempt.
- If `f` returns `nil`, return `nil` straight away.
- Before every attempt, and while waiting, watch the caller's `ctx`. If it is done, stop and return `ctx.Err()` without calling `f` again.
- After a failed attempt that is not the last, wait `backoff` (a wait of `0` does not wait at all) and try again.
- After the last attempt fails, return an error whose text starts with `after N attempts: ` (N is the number of attempts made) and which wraps the last error from `f`, so `errors.Is` finds it.

A failed attempt whose own timeout expired is an ordinary failure and is retried. Only the caller's context stops the whole loop.
