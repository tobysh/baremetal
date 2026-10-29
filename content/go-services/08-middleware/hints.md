`Chain`: loop **backwards** over `mws`: `for i := len(mws) - 1; i >= 0; i-- { h = mws[i](h) }`, then `return h`.
===
`RequireToken`: the closure returns `http.HandlerFunc(func(w, r) {...})`. Compute `want := "Bearer " + token` once, then `subtle.ConstantTimeCompare([]byte(r.Header.Get("Authorization")), []byte(want)) == 1`.
===
On failure: `w.Header().Set("WWW-Authenticate", "Bearer")`, then `http.Error(w, "unauthorized", http.StatusUnauthorized)` and `return`. On success `next.ServeHTTP(w, r)`.
===
`MaxBody`: `r.Body = http.MaxBytesReader(w, r.Body, n)` and then call `next`. Assigning to `r.Body` is fine because the server gives each handler its own request value.
