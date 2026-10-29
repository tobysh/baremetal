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
