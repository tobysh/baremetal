Write error handling for a tiny age parser:

- `ErrDivideByZero` is a sentinel error (`errors.New`). `Divide(a, b float64) (float64, error)` returns it when `b` is `0`.
- `RangeError` is a struct type with fields `Value`, `Min` and `Max`, with an `Error()` method on the pointer receiver that returns `"<Value> is outside <Min>..<Max>"`.
- `ParseAge(s string) (int, error)` converts `s` to an int. If it isn't a number, return an error that **wraps** the `strconv` error with `%w` and starts with `"parse age: "`. If the number is outside 0..150, return a `*RangeError`.
