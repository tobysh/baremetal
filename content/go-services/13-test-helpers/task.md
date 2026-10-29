Write these helpers. Each one must call `t.Helper()`.

- `AssertEqual[T comparable](t testing.TB, got, want T)` calls `t.Errorf("got %v, want %v", got, want)` when they differ, and does nothing otherwise.
- `AssertNoError(t testing.TB, err error)` calls `t.Fatalf("unexpected error: %v", err)` when `err != nil`.
- `AssertErrorIs(t testing.TB, err, target error)` calls `t.Errorf("got error %v, want %v", err, target)` unless `errors.Is(err, target)`. A nil `err` is a failure too (the message prints `<nil>`).
- `WithEnv(t testing.TB, key, value string)` sets the environment variable for the duration of the test and restores it afterwards using `t.Cleanup`: back to its old value, or unset again if it had none.
- `TempFile(t testing.TB, name, content string) string` writes `content` to a file called `name` inside `t.TempDir()` and returns the full path. A write error ends the test with `t.Fatalf`.
