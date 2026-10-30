`SafeAppend`: allocate a new slice with `make([]int, len(s), len(s)+1)`, `copy` the elements in, then `append` the value (there is room, so it uses the new array).
===
`Clone`: return `nil` when `s == nil`, otherwise `out := make([]int, len(s))`, `copy(out, s)`.
===
`Window` is one line with a full slice expression: `s[from:to:to]`.
===
`RemoveAt`: `return append(s[:i], s[i+1:]...)`. The tail is copied down over the removed element.
