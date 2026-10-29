Implement a `Group`:

- `WithContext(ctx context.Context) (*Group, context.Context)` returns a new group and a context derived from `ctx`. That context is cancelled the first time a task returns a non-nil error, or when `Wait` returns, whichever comes first.
- `(*Group).Go(f func() error)` runs `f` in a new goroutine.
- `(*Group).Wait() error` waits for **every** task started with `Go`, cancels the group's context, and returns the **first** non-nil error returned by any task (or `nil` if none failed). Later errors are ignored.

Use only the standard library.
