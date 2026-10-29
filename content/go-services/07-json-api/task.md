Build a small to-do API as a `Store` that serves HTTP. `Todo` is defined in the starter.

`NewStore() *Store` and `(*Store) Handler() http.Handler` with these routes (use a `ServeMux` with method patterns):

- `POST /todos` with body `{"title":"..."}`. Unknown fields or invalid JSON give 400 and `{"error":"invalid json"}`. A missing or blank (only spaces) title gives 400 and `{"error":"title required"}`. On success respond **201** with the new todo as JSON and a `Location: /todos/<id>` header. IDs start at 1 and count up. New todos are not done.
- `GET /todos` responds 200 with a JSON array of all todos in creation order. With no todos it is `[]`, never `null`.
- `GET /todos/{id}` responds 200 with the todo, or 404 with `{"error":"not found"}` (also for an id that is not a number).
- `POST /todos/{id}/done` marks the todo done and responds 200 with it, or 404 as above.

Every response from these routes has `Content-Type: application/json`. The store will be called from many goroutines, so protect it.
