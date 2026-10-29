---
title: Test Helpers with testing.TB
summary: Write assertion and setup helpers that report failures at the caller's line, using t.Helper, t.Cleanup and t.TempDir.
minutes: 40
difficulty: hard
tags: testing, generics, helpers
id: go-services/test-helpers
mode: harness
---
Once you have a dozen tests, the same five lines of checking appear everywhere. A **helper** removes the repetition, but only if failures still point at the right place.

```go
func assertEqual[T comparable](t testing.TB, got, want T) {
	t.Helper() // report failures at the CALLER's line, not in here
	if got != want {
		t.Errorf("got %v, want %v", got, want)
	}
}
```

## testing.TB

`testing.TB` is the interface shared by `*testing.T` (tests) and `*testing.B` (benchmarks). Take it as your parameter, and the helper works in both.

## The pieces

- **`t.Helper()`** marks the function as a helper. When something fails inside it, Go reports the line of the call in the test instead of a line in the helper.
- **`t.Errorf`** records a failure and carries on; **`t.Fatalf`** records it and stops the test immediately (it calls `runtime.Goexit`, so deferred functions still run). Use `Fatalf` when continuing makes no sense, such as after an unexpected error that leaves a nil result.
- **`t.Cleanup(f)`** runs `f` when the test ends, last-in first-out, even after a failure. It beats `defer` in helpers, because a `defer` inside the helper would run when the helper returns, not when the test does.
- **`t.TempDir()`** returns a fresh directory that is deleted automatically.

## Testing the helpers

How do you test a function that is meant to fail? Give it a **fake** `testing.TB`. Embed the interface in a struct and override only the methods you care about, recording what was called. The hidden tests for this lesson do exactly that.
