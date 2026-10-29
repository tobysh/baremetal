`Timeout`: `ctx, cancel := context.WithTimeout(r.Context(), d); defer cancel(); next.ServeHTTP(w, r.WithContext(ctx))`.
===
`RequestID`: `id := r.Header.Get("X-Request-ID"); if id == "" { id = gen() }`. Then `r = r.WithContext(context.WithValue(r.Context(), idKey{}, id))` and set the response header **before** calling `next`.
===
`IDFrom`: `id, _ := ctx.Value(idKey{}).(string); return id`, using `type idKey struct{}` as the key type.
===
`Lookup`: after `v, err := find(r.Context(), key)`, use `switch { case err == nil: ...; case errors.Is(err, ErrNotFound): ...; case errors.Is(err, context.DeadlineExceeded): ...; case errors.Is(err, context.Canceled): ... }`. `http.Error(w, msg, 499)` works with any number.
