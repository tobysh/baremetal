---
title: A JSON API
summary: Decode request bodies and encode responses as JSON, with correct status codes, headers and error bodies.
minutes: 40
difficulty: hard
tags: http, json, api
id: go-services/json-api
mode: harness
---
Most Go services speak JSON. A handler has three jobs: decode the request, do the work, and encode a response with the right **status code** and **Content-Type**.

```go
func writeJSON(w http.ResponseWriter, status int, v any) {
	w.Header().Set("Content-Type", "application/json")
	w.WriteHeader(status)
	json.NewEncoder(w).Encode(v) // adds a trailing newline
}
```
Remember the order: header first, then status, then body.

## Decoding strictly

```go
dec := json.NewDecoder(r.Body)
dec.DisallowUnknownFields() // a typo'd field is an error, not silently ignored
var in struct{ Title string `json:"title"` }
if err := dec.Decode(&in); err != nil {
	writeJSON(w, http.StatusBadRequest, map[string]string{"error": "invalid json"})
	return
}
```
`json.NewDecoder(r.Body)` streams from the body; `json.Unmarshal` would need `io.ReadAll` first.

## Status codes that clients rely on

- `201 Created` plus a `Location` header after creating something.
- `400` for input the client got wrong, `404` for a missing resource.
- Errors should have a consistent JSON shape, such as `{"error":"..."}`, so clients can parse them.

Shared state in a handler is accessed by many goroutines at once, since the server runs each request in its own. Guard it with a mutex.

## A worked request

Here is what a correct create and a bad create look like on the wire:

```text
POST /todos          {"title":"write docs"}
-> 201 Created
   Content-Type: application/json
   Location: /todos/1
   {"id":1,"title":"write docs","done":false}

POST /todos          {"titel":"typo"}
-> 400 Bad Request
   Content-Type: application/json
   {"error":"invalid json"}

GET /todos/99
-> 404 Not Found
   {"error":"not found"}
```

## Pitfalls

- **Header ordering.** After `WriteHeader` (or the first `Write`) the headers are already on the wire. A `w.Header().Set("Location", ...)` after `writeJSON` is silently ignored.
- Calling `WriteHeader` twice logs "superfluous response.WriteHeader call" and the second status is lost. Return right after writing an error.
- A nil slice encodes as `null`, an empty non-nil slice as `[]`. Clients usually expect the array.
- Never hand out a pointer into shared state: copy the value under the lock, then encode after unlocking, so a slow client cannot hold the mutex.
- Decode errors can leak internals; answer with a fixed message rather than `err.Error()`.

## When to use it

Every JSON endpoint needs this shape. Put `writeJSON` and `writeError` in one place so that status codes, headers and error bodies stay uniform across the whole API.
