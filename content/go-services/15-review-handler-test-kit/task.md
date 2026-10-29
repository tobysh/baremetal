- `DoJSON(h http.Handler, method, target string, body any) *httptest.ResponseRecorder` sends a request to `h` and returns the recorder. If `body` is `nil` the request has no body and no `Content-Type`. Otherwise the body is `body` encoded as JSON and the request has `Content-Type: application/json`. (Panic if `body` cannot be encoded: that is a mistake in the test itself.)
- `DecodeBody[T any](t testing.TB, rec *httptest.ResponseRecorder) T` decodes the recorder's body as JSON into a `T` and returns it. If the body is not valid JSON for `T`, call `t.Fatalf("bad JSON body: %v\nbody: %s", err, body)` and return the zero value.
- `AssertStatus(t testing.TB, rec *httptest.ResponseRecorder, want int)` calls `t.Errorf("status = %d, want %d; body: %s", rec.Code, want, rec.Body.String())` when the status differs.
- `AssertJSONError(t testing.TB, rec *httptest.ResponseRecorder, status int, msg string)` checks the status with `AssertStatus`, and then checks that the body is `{"error": ...}` with the text `msg`, reporting a mismatch with `t.Errorf("error = %q, want %q", got, msg)`.

All four call `t.Helper()` first.
