Write `writeJSON(w, status, v)` first, as in the lesson, and `writeError(w, status, msg)` that calls it with `map[string]string{"error": msg}`. Every route then becomes a few lines.
===
The `Store` needs a `sync.Mutex`, a `[]Todo` and the next id. Lock inside each handler around the part that touches the slice, and copy the todo before unlocking and writing it.
===
For `GET /todos`: `out := make([]Todo, len(s.todos))` then `copy`. A non-nil empty slice encodes as `[]`.
===
For the id routes: `id, err := strconv.Atoi(r.PathValue("id"))`; on error or if `id < 1 || id > len(s.todos)` answer 404. Because ids count up from 1 with no deletion, the todo is `s.todos[id-1]`.
===
In `POST /todos` decode with `json.NewDecoder(r.Body)` + `DisallowUnknownFields()`, check `strings.TrimSpace(in.Title) == ""`, and set `w.Header().Set("Location", ...)` **before** `writeJSON(w, 201, ...)`.
