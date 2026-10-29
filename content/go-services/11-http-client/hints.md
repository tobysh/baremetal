`RoundTripFunc` needs the method `func (f RoundTripFunc) RoundTrip(r *http.Request) (*http.Response, error) { return f(r) }`. `NewTestClient` returns `&http.Client{Transport: fn}`.
===
`HTTPError.Error`: `fmt.Sprintf("http %d: %s", e.Status, e.Body)`. Return it as a pointer, `&HTTPError{...}`, so callers can use `errors.As(err, &he)` with `var he *HTTPError`.
===
Share the work: write `do(c, req, out)` that calls `c.Do(req)`, `defer resp.Body.Close()`, checks the status, and decodes. `FetchJSON` and `PostJSON` just build the request.
===
For the error body: `b, _ := io.ReadAll(io.LimitReader(resp.Body, 100))`. For `PostJSON`, `json.Marshal(in)` and wrap the bytes with `bytes.NewReader`.
