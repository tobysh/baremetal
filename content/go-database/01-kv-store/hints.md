Use `data map[string][]byte` inside the struct and create it in `NewStore`, otherwise writing to a nil map panics.
===
Copy with `c := make([]byte, len(v)); copy(c, v)`. Using `append([]byte(nil), v...)` yields `nil` for an empty value, which is fine to store, but `make` keeps it simple.
===
Presence: `v, ok := s.data[key]`. For `Delete`, check `ok` first, then call `delete(s.data, key)`.
===
`Keys`: `keys := make([]string, 0, len(s.data))`, append each key from a `range`, then `sort.Strings(keys)`.
