`DoJSON`: `var r io.Reader` stays nil without a body; otherwise `b, err := json.Marshal(body)`, `panic(err)` on failure, and `r = bytes.NewReader(b)`. Then `req := httptest.NewRequest(method, target, r)`, set the header if there is a body, and `h.ServeHTTP(rec, req)` with `rec := httptest.NewRecorder()`.
===
Watch out for the typed-nil trap: only assign `r` when the body is not nil, so `httptest.NewRequest` really gets a nil reader.
===
`DecodeBody`: `var v T`, `if err := json.Unmarshal(rec.Body.Bytes(), &v); err != nil { t.Fatalf(...); }`, `return v`. The `return` after `Fatalf` is needed for the compiler even though a real `Fatalf` never comes back.
===
`AssertJSONError`: call `AssertStatus(t, rec, status)`, then `var e struct{ Error string `json:"error"` }`, unmarshal (use `t.Fatalf` if that fails), and compare `e.Error` with `msg`.
