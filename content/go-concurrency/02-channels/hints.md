`Gen`: `ch := make(chan int)`, then `go func() { defer close(ch); for i := 1; i <= n; i++ { ch <- i } }()`, then `return ch`.
===
`Sum` is `for v := range ch { total += v }`.
===
`Preloaded`: `ch := make(chan int, len(vals))`, send every value (the buffer has room, so nothing blocks), then `close(ch)` and return it.
===
`Take`: loop `for len(out) < n { v, ok := <-ch; if !ok { break }; out = append(out, v) }`. Start with `out := []int{}` so the result is never nil.
