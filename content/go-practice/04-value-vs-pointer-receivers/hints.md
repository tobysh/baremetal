Change the receiver of `Inc` to `*Counter` and write `c.n++`. `Value` can stay a value receiver, but it does no harm to leave it.
===
`IncAll`: `for i := range cs { cs[i].Inc() }`. With `for _, c := range cs` you would increment a copy.
===
`Scale` needs `func (r *Rect) Scale(f float64)` with `r.W *= f` and `r.H *= f`. Leave `Area` as `func (r Rect) Area() float64`.
===
`Stack`: give all three methods the receiver `*Stack`. `Push` is `s.items = append(s.items, v)`. `Pop` returns `0, false` for an empty stack; otherwise read `s.items[len(s.items)-1]` and shrink with `s.items = s.items[:len(s.items)-1]`.
===
`Fill`: `for i := 1; i <= n; i++ { p.Push(i) }`. The tests pass a `*Stack`, which has `Push` in its method set.
