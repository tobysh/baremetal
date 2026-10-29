Start with `ctx, cancel := context.WithCancel(ctx)` and `defer cancel()`. Make `results := make([]string, len(jobs))` and a buffered `sem` channel of size `limit`.
===
Record the first error with a mutex (or `sync.Once`): only if `firstErr == nil`, set it, then call `cancel()`. Cancel *before* releasing the semaphore slot, so the main loop sees the cancellation when it gets the slot.
===
In the main loop: `select { case sem <- struct{}{}: case <-ctx.Done(): break loop }`, then `if ctx.Err() != nil { <-sem; break }`.
===
At the end: `wg.Wait()`, then return `results` with the first job error if any; otherwise `parent.Err()` if some jobs were skipped. Keep the original context in a separate variable (for example `parent := ctx`) before you shadow it.
