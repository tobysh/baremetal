Implement these functions (a panic you do not recover will fail the test):

- `SafeDivide(a, b int) (result int, err error)` returns `a / b`. Integer division by zero panics in Go; recover from it and return `0` and a non-nil error whose text contains `"divide by zero"`. Use **named results**.
- `Try(f func()) error` calls `f` and returns `nil` if it finishes normally. If `f` panics, `Try` recovers and returns an error: when the panic value is already an `error`, return that same error; when it is a `string`, return `errors.New` of it; for any other value, return `fmt.Errorf("panic: %v", v)`.
- `DeferOrder(names ...string) (ran []string)` defers, for each name in turn, a call that appends that name to `ran`, and returns. The deferred calls run after the `return`, so `DeferOrder("a", "b", "c")` returns `["c" "b" "a"]`.
- `WithCleanup(work func() error, cleanup func()) error` runs `work` and returns its error. `cleanup` must run **exactly once** afterwards, whether `work` returned normally, returned an error, or panicked. If `work` panics, the panic must keep going up (do not recover it).
