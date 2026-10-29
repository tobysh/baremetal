`NewAbortable`: `ctx, cancel := context.WithCancelCause(parent)` and return `ctx, func(reason string) { cancel(errors.New(reason)) }`.
===
`Why`: `err := context.Cause(ctx)`, then `switch { case err == nil: return ""; case err == context.Canceled: ...; case err == context.DeadlineExceeded: ...; default: return err.Error() }`. Use `errors.Is` if you prefer.
===
`TimeoutWithCause`: `context.WithTimeoutCause(parent, d, fmt.Errorf("%s timed out", what))`.
===
`OnCancel` is `return context.AfterFunc(ctx, f)`.
