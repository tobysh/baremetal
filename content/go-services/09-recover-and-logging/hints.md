`statusWriter.WriteHeader`: `if !w.wrote { w.status = code; w.wrote = true }` then `w.ResponseWriter.WriteHeader(code)`. `Write`: `if !w.wrote { w.WriteHeader(http.StatusOK) }`, then `n, err := w.ResponseWriter.Write(b); w.bytes += n; return n, err`.
===
In `AccessLog` create `sw := &statusWriter{ResponseWriter: w}`, call `next.ServeHTTP(sw, r)`, then `status := sw.status` and use 200 if `!sw.wrote`.
===
`Recover` also wraps the writer in a `statusWriter` so it can ask `sw.wrote`. Use `defer func() { v := recover(); if v == nil { return }; if v == http.ErrAbortHandler { panic(v) }; report(v); if !sw.wrote { http.Error(sw, "internal server error", 500) } }()`.
===
Use `sw` (not `w`) when writing the 500, so an outer `AccessLog` sees it through its own wrapper. Both wrappers chain because each one calls the writer it wraps.
