---
title: An HTTP Client You Can Test
summary: Write a JSON client with contexts and typed errors, and test it by swapping the http.Client's Transport for a function.
minutes: 40
difficulty: hard
tags: http, client, testing
id: go-services/http-client
mode: harness
---
Code that calls other services is hard to test if every test needs a real server. The trick is a seam: `http.Client` sends requests through its `Transport`, an interface with one method.

```go
type RoundTripper interface {
	RoundTrip(*http.Request) (*http.Response, error)
}
```
Define a function type that implements it, the same trick as `http.HandlerFunc`:

```go
type RoundTripFunc func(*http.Request) (*http.Response, error)

func (f RoundTripFunc) RoundTrip(r *http.Request) (*http.Response, error) { return f(r) }
```
Now a test can build a client whose "network" is a function and see exactly what request was sent:

```go
c := &http.Client{Transport: RoundTripFunc(func(r *http.Request) (*http.Response, error) {
	return &http.Response{StatusCode: 200, Body: io.NopCloser(strings.NewReader(`{"n":1}`)), Header: http.Header{}}, nil
})}
```

## Client rules worth remembering

- Build requests with `http.NewRequestWithContext(ctx, ...)` so cancellation and deadlines work.
- **Always close the response body**, usually with `defer resp.Body.Close()`, or connections leak.
- `Do` succeeds for a 404 or 500: those are *responses*. You must check `resp.StatusCode` yourself and turn bad statuses into errors.
- Read at most a bounded amount of an error body (`io.LimitReader`) so a hostile server cannot fill your memory.
- `Do` wraps transport errors in `*url.Error`, which supports `errors.Is`, so `errors.Is(err, context.Canceled)` still works.
