---
title: Errors are values
summary: Handle failure the Go way: return an error, check it, wrap it with %w, and test it with errors.Is and errors.As.
minutes: 22
difficulty: medium
id: go-foundations/errors
mode: harness
---
Go has no exceptions for ordinary failures. A function that can fail returns an `error` as its last result, and the caller checks it:

```go
n, err := strconv.Atoi("12x")
if err != nil {
	return 0, err          // pass it up
}
```

`error` is a tiny built-in interface, `interface{ Error() string }`, and `nil` means success. You will write `if err != nil` a lot. That is the design: the failure path is visible, not hidden.

## Creating errors

```go
var ErrEmpty = errors.New("empty input")           // a sentinel error: compare it by identity
err := fmt.Errorf("parsing %q: bad digit", s)       // formatted
```

## Wrapping

To add context but keep the original cause, wrap with `%w`:

```go
return 0, fmt.Errorf("read config: %w", err)
```

The wrapped chain can be inspected anywhere up the call stack:

```go
if errors.Is(err, ErrEmpty) { ... }                // is ErrEmpty anywhere in the chain?

var numErr *strconv.NumError
if errors.As(err, &numErr) {                       // is there a *strconv.NumError? Fill numErr.
	fmt.Println(numErr.Num, numErr.Err)
}
```

Never compare with `==` on a wrapped error; use `errors.Is`. Write `errors.As` with a pointer to a variable of the error type you look for.

## Custom error types

Any type with an `Error() string` method is an error, and it can carry data:

```go
type RangeError struct{ Value, Min, Max int }

func (e *RangeError) Error() string {
	return fmt.Sprintf("%d is outside %d..%d", e.Value, e.Min, e.Max)
}
```

## panic is not an error

`panic` is for programmer mistakes and impossible states (like indexing past a slice). Expected failures, such as bad user input, are returned as errors.
