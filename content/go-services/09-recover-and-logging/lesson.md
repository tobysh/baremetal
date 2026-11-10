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

## A worked trace

Handler panics before writing anything:

```text
GET /boom
  AccessLog in
    Recover in
      handler: panic("nil map")
    Recover: recover() != nil, sw.wrote == false
             logs the panic and stack, writes 500
  AccessLog out -> logs "GET /boom 500"
```

Handler panics after writing:

```text
GET /half
  handler: writes "partial", status 200 already sent
  handler: panic
  Recover: wrote == true, so it logs but must not call WriteHeader again
```

## Pitfalls

- Forgetting that `recover` only works **directly inside a deferred function**. Calling a helper that calls `recover` does nothing.
- Recovering and then writing a second status: the server prints "superfluous WriteHeader" and the client sees the first status.
- Swallowing `http.ErrAbortHandler`. It is how handlers abort a stream on purpose, and the server expects it to propagate.
- Logging `r.URL.Path` and a status that your wrapper never captured because the handler used the implicit 200 from `Write`.
- Panics in goroutines the handler started are **not** caught by this middleware; only the request goroutine is.

## When to use it

Put `Recover` and `AccessLog` as the outermost middleware in every service, so one bad request cannot crash the process and every response, including the 500s, is logged.
