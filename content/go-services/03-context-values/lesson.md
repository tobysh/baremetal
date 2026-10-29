---
title: Values and the Context Tree
summary: Carry request-scoped data in a context safely, and understand how cancellation flows from parent to child and never back.
minutes: 30
difficulty: medium
tags: context, values
id: go-services/context-values
mode: harness
---
Every `With...` function returns a **child** of the context you gave it. Together they form a tree.

- Cancelling a context cancels all of its descendants.
- Cancelling a child never affects its parent or its siblings.
- A child sees the values of its ancestors.

## Values

```go
ctx = context.WithValue(ctx, key, "abc")
v, ok := ctx.Value(key).(string)
```
Use values only for request-scoped data that crosses API boundaries, such as a request ID or the authenticated user. Do not use them to pass optional function arguments.

**Keys must not collide.** If two packages both use the string `"id"` they overwrite each other. The convention is an unexported type of your own:

```go
type ctxKey int

const requestIDKey ctxKey = iota
```
Because the type is unexported and different from `string`, no one else can produce an equal key. Wrap the access in small functions so callers never see the key at all.

## Detaching

Sometimes work must outlive the request that started it, such as writing an audit record after the client hung up. `context.WithoutCancel(ctx)` keeps the values but drops the cancellation and the deadline.
