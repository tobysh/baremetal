Complete the package:

- `ErrNotFound` is a sentinel error made with `errors.New("not found")`.
- `ValidationError` has `Field` and `Reason` strings. Its `Error()` method (pointer receiver) returns `Field + ": " + Reason`.
- `Lookup(db map[string]int, key string) (int, error)` returns the value, or `0` and an error that wraps `ErrNotFound` and whose text starts with `lookup "KEY": ` (use `%q` for the key).
- `Validate(name string, age int) error` returns `nil` when valid. An empty `name` gives a `*ValidationError` with `Field: "name"` and `Reason: "must not be empty"`. Otherwise an `age` outside `0..150` gives `Field: "age"` and `Reason: "must be between 0 and 150"`. The name is checked first.
- `Describe(err error) string` returns `"ok"` for `nil`; `"missing"` if the chain contains `ErrNotFound`; `"invalid " + Field + " (" + Reason + ")"` if it contains a `*ValidationError`; otherwise `"error: " + err.Error()`. It must work when the error has been wrapped further by the caller.
