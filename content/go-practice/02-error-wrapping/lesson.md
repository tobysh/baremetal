---
title: Wrapping and Inspecting Errors
summary: Add context to errors with %w, then find the cause with errors.Is and errors.As.
minutes: 30
difficulty: medium
tags: errors
id: go-practice/error-wrapping
mode: harness
---
In [Errors](/learn/go-foundations/errors) you met errors as ordinary values. Real programs add one more habit: each layer adds **context** without throwing away the original cause.

## Wrapping with %w

```go
var ErrNotFound = errors.New("not found")

func lookup(db map[string]int, key string) (int, error) {
	v, ok := db[key]
	if !ok {
		return 0, fmt.Errorf("lookup %q: %w", key, ErrNotFound)
	}
	return v, nil
}
```

`%w` (not `%v`) wraps: the new error's text is `lookup "x": not found`, and it still *contains* `ErrNotFound`. Callers can look inside.

## errors.Is: is this (or something it wraps) that error?

```go
if errors.Is(err, ErrNotFound) { ... }
```

`err == ErrNotFound` would be false for the wrapped error above. `errors.Is` walks the chain of wrapped errors.

## errors.As: is there an error of this type in the chain?

Your own error types can carry detail:

```go
type ValidationError struct {
	Field  string
	Reason string
}

func (e *ValidationError) Error() string { return e.Field + ": " + e.Reason }

var ve *ValidationError
if errors.As(err, &ve) {
	fmt.Println("bad field:", ve.Field)
}
```

`errors.As` needs a pointer to a variable of the type you are looking for; if it finds one in the chain it stores it there and returns `true`.

## Rules of thumb

- Wrap with context where it helps the reader ("loading config: ..."), but don't repeat what the cause already says.
- Compare with `errors.Is`/`errors.As`, never with `==` or by searching the message text.
- Return a nil `error`, not a typed nil pointer: `return nil`, not `return (*ValidationError)(nil)`.
