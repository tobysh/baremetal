`SafeDivide`: start with `defer func() { if r := recover(); r != nil { err = fmt.Errorf("recovered: %v", r) } }()`, then `return a / b, nil`. Because `result` and `err` are named, the closure can set `err`. After a recovered panic `result` is still 0.
===
`Try`: the same pattern with a type switch on the recovered value: `switch v := r.(type) { case error: err = v; case string: err = errors.New(v); default: err = fmt.Errorf("panic: %v", v) }`.
===
`DeferOrder`: `for _, n := range names { defer func() { ran = append(ran, n) }() }`, then a plain `return`. Deferred calls run last-in first-out, which reverses the order. (Since Go 1.22 each iteration has its own `n`.)
===
`WithCleanup`: `defer cleanup()` first, then `return work()`. A deferred call runs during a panic as well, and as you never call `recover` the panic carries on.
