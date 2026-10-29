Use `Middleware` from the starter (`func(http.Handler) http.Handler`).

- Finish `statusWriter`: `WriteHeader` records the first status only, `Write` counts the bytes written and records an implicit 200 if no status was set, and `Unwrap() http.ResponseWriter` returns the wrapped writer.
- `AccessLog(log func(method, path string, status, bytes int)) Middleware` calls the next handler with a `statusWriter`, and **afterwards** calls `log` once with the request method, `r.URL.Path`, the final status (200 if the handler wrote nothing at all) and the number of body bytes written.
- `Recover(report func(v any)) Middleware` calls the next handler; if it panics, call `report` with the panic value and, if no response has been started, answer with status 500 and the body `internal server error`. If a response was already started, only report it. A panic with `http.ErrAbortHandler` is re-panicked after reporting nothing.

When both are chained as `Chain(h, AccessLog(...), Recover(...))`, the log must show status 500 for a panicking handler. That means `Recover` has to write the 500 itself instead of letting the panic escape.
