`Map` and `Filter`: start with `out := make([]U, 0, len(xs))` (or `[]T`), so an empty input gives an empty non-nil slice, then `append` in a loop.
===
`Reduce`: `acc := init`, loop with `acc = f(acc, x)`, return `acc`.
===
`Max`: handle `len(xs) == 0` first using `var zero T`. Then keep a `best` that starts as `xs[0]`.
===
`Stack`: a struct with an `items []T` field. `Pop` takes `s.items[len(s.items)-1]` and shortens the slice with `s.items = s.items[:len(s.items)-1]`.
