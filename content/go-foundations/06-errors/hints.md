`Divide`: `if b == 0 { return 0, ErrDivideByZero }`. Declare the sentinel with `var ErrDivideByZero = errors.New("divide by zero")`.
===
`RangeError.Error()` can be `fmt.Sprintf("%d is outside %d..%d", e.Value, e.Min, e.Max)`; remember the pointer receiver `(e *RangeError)`.
===
In `ParseAge`: `n, err := strconv.Atoi(s)`; on error `return 0, fmt.Errorf("parse age: %w", err)`. Then if `n < 0 || n > 150` return `0, &RangeError{n, 0, 150}`. Imports: errors, fmt, strconv.
