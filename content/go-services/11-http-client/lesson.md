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

## A worked example

The whole point is that a test sees the exact request and controls the answer:

```go
var got *http.Request
c := NewTestClient(func(r *http.Request) (*http.Response, error) {
	got = r
	return &http.Response{
		StatusCode: 500,
		Header:     http.Header{},
		Body:       io.NopCloser(strings.NewReader("boom")),
	}, nil
})
err := FetchJSON(ctx, c, "http://api.test/items/7", &out)
// got.Method == "GET", got.URL.Path == "/items/7"
// errors.As(err, &he) with he.Status == 500, he.Body == "boom"
```

```text
200 {"n":1}        -> decoded into out, nil error
404 "nope"         -> *HTTPError{404, "nope"}
transport error    -> wrapped *url.Error, errors.Is(err, context.Canceled) still works
```

## Pitfalls

- **Leaked connections.** Forgetting `resp.Body.Close()`, or closing without reading, stops the connection being reused.
- Treating `err == nil` from `Do` as success. A 500 is a successful round trip.
- `http.DefaultClient` has **no timeout**. A stuck server blocks you forever, so always pass a context with a deadline or set `Client.Timeout`.
- Reading an unbounded error body into memory. Cap it with `io.LimitReader`.
- A fake transport must return a non-nil `Body`, or `Close` panics.

## When to use it

Swap the `Transport` for tests of anything that calls another service. For slow end-to-end checks `httptest.NewServer` is still useful, but most client logic needs only this seam.
