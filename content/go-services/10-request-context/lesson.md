---
title: Request Contexts
summary: Use r.Context() to honour client disconnects and deadlines, map context errors to status codes, and pass request IDs down.
minutes: 40
difficulty: hard
tags: http, context, middleware
id: go-services/request-context
mode: harness
---
Every `*http.Request` carries a context: `r.Context()`. The server cancels it when the client disconnects, when the request finishes, or when the connection breaks. Passing it down to your database and API calls means abandoned requests stop wasting work.

```go
func handler(w http.ResponseWriter, r *http.Request) {
	val, err := find(r.Context(), key)
	...
}
```

## Changing the context

Requests are immutable by convention. To attach a deadline or a value, make a copy with the new context:

```go
ctx, cancel := context.WithTimeout(r.Context(), 2*time.Second)
defer cancel()
next.ServeHTTP(w, r.WithContext(ctx))
```

## Mapping errors to status codes

The context errors say what happened, and the handler decides what the client sees:

- `context.DeadlineExceeded` means we were too slow: **504 Gateway Timeout**.
- `context.Canceled` means the client went away. Nobody reads the response, but logs and metrics should still say so. A common choice is the non-standard **499**.
- A "not found" sentinel error becomes **404**; anything else is **500**.

Test this without a network: `httptest.NewRequestWithContext(ctx, ...)` builds a request whose context you control, for example one that is already cancelled.

## A worked example

A handler that does slow work must hand the request context down, so that giving up on the request stops the work:

```go
func slow(w http.ResponseWriter, r *http.Request) {
	select {
	case <-time.After(5 * time.Second): // pretend work
		fmt.Fprintln(w, "done")
	case <-r.Context().Done():
		// client left or a Timeout middleware fired
		return
	}
}
```

Behind `Timeout(100ms)` this handler returns after 100 ms instead of five seconds, and the goroutine and its database connection are released.

```text
GET /lookup?key=z   find returns ErrNotFound      -> 404
GET /lookup?key=a   find returns "apple"          -> 200 apple
GET /lookup?key=a   find returns DeadlineExceeded -> 504
GET /lookup?key=a   client already gone           -> 499 (logged only)
```

## Pitfalls

- **Leaked work.** Using `context.Background()` inside a handler ignores cancellation entirely, which is the bug behind many stuck connections.
- Starting a goroutine that outlives the request while still using `r.Context()`: the context is cancelled when the handler returns.
- Missing `defer cancel()` after `WithTimeout` leaks a timer until it fires.
- Using a string as the context key collides with other packages. Use an unexported struct type.
- Writing a response after `Canceled` is wasted effort; check `errors.Is`, not `==`, because errors get wrapped.

## When to use it

Use `r.Context()` for every call that leaves the handler: database queries, outgoing HTTP, queue publishes. Use a value only for request-scoped data such as an id, never for optional parameters.
