`Lazy`: fields `once sync.Once` and `val int`. In `Get`: `l.once.Do(func() { l.val = init() })` then `return l.val`.
===
`Stats` can have three fields of type `atomic.Int64`: count, total and max. `Record` does `s.count.Add(1)` and `s.total.Add(v)`.
===
`Max` needs a compare-and-swap loop: load the current max; if `v` is not bigger, stop; otherwise try `CompareAndSwap(cur, v)` and, if it fails, loop and look again.
===
Do not write `if v > s.max.Load() { s.max.Store(v) }`. Two goroutines can both pass the check, and the smaller one may store last.
