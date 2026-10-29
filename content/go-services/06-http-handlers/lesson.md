---
title: HTTP Handlers and Routing
summary: Write net/http handlers, route with ServeMux patterns, and test them in-process with httptest.
minutes: 35
difficulty: medium
tags: http, httptest, routing
id: go-services/http-handlers
mode: harness
---
A Go web server is built from one interface:

```go
type Handler interface {
	ServeHTTP(w http.ResponseWriter, r *http.Request)
}
```
`http.HandlerFunc` turns an ordinary function with that signature into a `Handler`. A handler reads the request `r` and writes the response through `w`.

```go
func hello(w http.ResponseWriter, r *http.Request) {
	name := r.URL.Query().Get("name")
	w.Header().Set("Content-Type", "text/plain; charset=utf-8")
	w.WriteHeader(http.StatusOK) // optional: the first Write sends 200
	fmt.Fprintf(w, "Hello, %s!\n", name)
}
```
**Order matters**: headers must be set *before* `WriteHeader` or the first `Write`, because after that they are already on the wire. `http.Error(w, "msg", code)` sets the content type, status and body in one call.

## Routing

`http.ServeMux` (Go 1.22+) matches the method and path together, with wildcards:

```go
mux := http.NewServeMux()
mux.HandleFunc("GET /users/{id}", func(w http.ResponseWriter, r *http.Request) {
	id := r.PathValue("id")
	...
})
```
A request that matches a path but not its method gets `405 Method Not Allowed` and an `Allow` header automatically; an unknown path gets `404`.

## Testing without a network

`net/http/httptest` lets you call a handler directly, with no socket at all:

```go
req := httptest.NewRequest("GET", "/users/7", nil)
rec := httptest.NewRecorder()
mux.ServeHTTP(rec, req)
rec.Code            // 200
rec.Body.String()   // the body
rec.Header()        // the headers
```
This is fast and deterministic, and it is how the hidden tests for this whole course exercise your servers.
