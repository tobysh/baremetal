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

## A worked trace

With `Chain(final, Log, Auth)` the request passes through `Log`, then `Auth`, then `final`, and the response comes back out the same way:

```text
GET /admin  (no token)
  Log  in
    Auth in   -> token missing: writes 401, does NOT call next
  Log  out    -> sees status 401
final never runs

GET /admin  Authorization: Bearer s3
  Log  in
    Auth in   -> ok, calls next
      final   -> writes 200 "secret"
    Auth out
  Log  out    -> sees status 200
```

## Pitfalls

- **Forgetting `return`** after writing a 401. The handler then falls through to `next.ServeHTTP` and the protected code runs anyway.
- **Header ordering.** A middleware that sets a header must do it before `next` writes the body. Setting it afterwards is too late, because the headers have already been sent.
- Building the chain per request instead of once at start-up wastes allocations and hides bugs. Compose once, serve many times.
- Order is behaviour: a rate limiter placed after authentication cannot protect the login path. Put logging and panic recovery outermost so they see everything.
- Comparing tokens with `==` leaks timing; use `subtle.ConstantTimeCompare`.

## When to use it

Use middleware for cross-cutting concerns that every route shares: authentication, request limits, common headers, logging, metrics. Logic that belongs to a single route stays in that route's handler.
