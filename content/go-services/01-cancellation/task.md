Write three cancellation-aware helpers.

- `Counter(ctx context.Context) <-chan int` returns an unbuffered channel that yields 0, 1, 2, ... forever. When `ctx` is cancelled the goroutine stops and the channel is **closed**.
- `Sleep(ctx context.Context, d time.Duration) error` waits for `d` and returns `nil`, but returns `ctx.Err()` immediately if the context is done first. Do not leave the timer running. If `ctx` is already cancelled it must return the error even when `d` is `0`.
- `Drain(ctx context.Context, in <-chan int) []int` collects values from `in` until `in` is closed **or** `ctx` is done, and returns what it got (an empty non-nil slice if nothing arrived).

The tests cancel contexts while your code is blocked, so a plain `time.Sleep` or an unguarded send will hang.
