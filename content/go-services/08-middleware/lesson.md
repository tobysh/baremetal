---
title: Middleware
summary: Wrap handlers to add authentication, headers and limits, and compose them in a predictable order.
minutes: 35
difficulty: hard
tags: http, middleware, closures
id: go-services/middleware
mode: harness
---
**Middleware** is a function that takes a handler and returns a new handler that does something before and/or after calling it. It is closures and interfaces, nothing more.

```go
type Middleware func(http.Handler) http.Handler

func Timing(next http.Handler) http.Handler {
	return http.HandlerFunc(func(w http.ResponseWriter, r *http.Request) {
		start := time.Now()
		next.ServeHTTP(w, r) // call the wrapped handler
		log.Println(r.URL.Path, time.Since(start))
	})
}
```
A middleware may also **stop** the request by simply not calling `next`: authentication does that when the token is wrong.

When a middleware needs settings, write a function that returns a Middleware:

```go
func RequireToken(token string) Middleware {
	return func(next http.Handler) http.Handler { ... }
}
```

## Order

`Chain(h, A, B, C)` should give `A(B(C(h)))`: A is outermost, so it sees the request **first** and the response last. Build it by wrapping from the last middleware inwards.

## Useful tools

- `subtle.ConstantTimeCompare` compares secrets without leaking, through timing, how many leading bytes matched.
- `http.MaxBytesReader(w, r.Body, n)` makes reads fail once the client sends more than `n` bytes. The failure is a `*http.MaxBytesError`.
