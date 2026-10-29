`AssertEqual`: first line `t.Helper()`, then `if got != want { t.Errorf("got %v, want %v", got, want) }`. The type parameter `T comparable` lets you use `!=`.
===
`AssertErrorIs`: `if !errors.Is(err, target) { t.Errorf("got error %v, want %v", err, target) }`. Printing a nil error with `%v` gives `<nil>` automatically.
===
`WithEnv`: `old, had := os.LookupEnv(key)`, then `os.Setenv(key, value)`, then `t.Cleanup(func() { if had { os.Setenv(key, old) } else { os.Unsetenv(key) } })`.
===
`TempFile`: `p := filepath.Join(t.TempDir(), name)`; `if err := os.WriteFile(p, []byte(content), 0o600); err != nil { t.Fatalf("writing %s: %v", p, err) }`; `return p`.
