`Middleware` is defined in the starter as `func(http.Handler) http.Handler`.

- `Chain(h http.Handler, mws ...Middleware) http.Handler` wraps `h` so that the **first** middleware in the list is the outermost one. With no middleware it returns `h`.
- `RequireToken(token string) Middleware` lets a request through only when its `Authorization` header is exactly `Bearer <token>`. Otherwise respond 401 with a `WWW-Authenticate: Bearer` header and do not call the next handler. Compare with `crypto/subtle`.
- `SetHeader(key, value string) Middleware` sets that header on the response and then calls the next handler (so the handler can still override it).
- `MaxBody(n int64) Middleware` limits the request body to `n` bytes with `http.MaxBytesReader`.
