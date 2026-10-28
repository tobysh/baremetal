`Lookup`: `return 0, fmt.Errorf("lookup %q: %w", key, ErrNotFound)`. The verb must be `%w`, or `errors.Is` can't see inside.
===
`Validate`: return `&ValidationError{Field: "name", Reason: "must not be empty"}`. A pointer, because `Error()` is on the pointer receiver.
===
`Describe`: check `err == nil` first, then `errors.Is(err, ErrNotFound)`, then declare `var ve *ValidationError` and test `errors.As(err, &ve)`.
