---
title: Recovering and Access Logs
summary: Wrap a ResponseWriter to learn the status code, then use it to log requests and turn handler panics into 500 responses.
minutes: 40
difficulty: hard
tags: http, middleware, panics
id: go-services/recover-and-logging
mode: harness
---
Some middleware needs to know what the handler **did**: which status it sent, how many bytes it wrote, whether it wrote anything at all. The `ResponseWriter` interface has no getter for those, so you wrap it:

```go
type statusWriter struct {
	http.ResponseWriter // embedded: everything else passes straight through
	status int
	wrote  bool
}

func (w *statusWriter) WriteHeader(code int) {
	if !w.wrote {
		w.status = code
		w.wrote = true
	}
	w.ResponseWriter.WriteHeader(code)
}
```
Careful with the implicit case: calling `Write` without `WriteHeader` sends **200**. Your `Write` method must record that too.

## Optional interfaces

A real writer may also support `Flush`, `Hijack` and so on. A wrapper hides them, unless it has an `Unwrap() http.ResponseWriter` method: `http.NewResponseController(w)` follows `Unwrap` to find them.

## Panics

A panic in one handler must not take the whole process down. The server recovers panics itself, but it just drops the connection. A `Recover` middleware does better:

```go
defer func() {
	if v := recover(); v != nil {
		// log it, and if nothing was sent yet, answer 500
	}
}()
```
Two rules: if the handler already sent headers you can no longer change the status, and a panic with `http.ErrAbortHandler` is the deliberate way to abort a response, so re-panic with it.
