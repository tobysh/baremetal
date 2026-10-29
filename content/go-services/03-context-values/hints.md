Declare `type ctxKey int` and two constants of it, for example `const (requestIDKey ctxKey = iota; userKey)`. Never use a plain `string` as a key.
===
`RequestID`: `id, ok := ctx.Value(requestIDKey).(string); return id, ok`. A missing key gives a nil interface, and the type assertion safely yields `"", false`.
===
`User`: `if u, ok := ctx.Value(userKey).(string); ok { return u }; return "anonymous"`. `Describe` builds on these two (`fmt.Sprintf`).
===
`Detach` is a one-liner: `context.WithoutCancel(ctx)`.
