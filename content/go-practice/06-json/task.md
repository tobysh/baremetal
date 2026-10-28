`Item` is defined in the starter. Add the struct tags so that its JSON keys are `name`, `price` and `tags`, with `tags` left out when empty. The unexported field `internal` must never appear in the JSON.

- `Encode(items []Item) (string, error)` returns the compact JSON array for `items`. A `nil` or empty slice gives `[]`.
- `Pretty(items []Item) (string, error)` returns the same array indented with two spaces (no prefix).
- `Decode(s string) ([]Item, error)` parses a JSON array of items. Invalid JSON returns `nil` and the error.
- `Total(s string) (float64, error)` decodes the JSON array `s` and returns the sum of the prices (an error for invalid JSON).
