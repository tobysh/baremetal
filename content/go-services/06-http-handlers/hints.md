`Greet`: `if r.Method != http.MethodGet { w.Header().Set("Allow", "GET"); http.Error(w, "method not allowed", http.StatusMethodNotAllowed); return }`.
===
Then `name := r.URL.Query().Get("name")`, default it to `"world"`, set the `Content-Type` header, and `fmt.Fprintf(w, "Hello, %s!\n", name)`.
===
`NewMux`: `mux := http.NewServeMux()` and register `"GET /greet"` with `mux.HandleFunc("GET /greet", Greet)`. Pass `Greet` itself, not `Greet(...)`.
===
For the id: `id := r.PathValue("id")`, then check every byte is `'0' <= c && c <= '9'` (an empty id cannot occur, but `strconv.Atoi` would wrongly accept `-5` and `+5`, so write the loop).
