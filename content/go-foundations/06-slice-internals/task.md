Implement these functions. The tests look at which slices share memory, so follow each description closely.

- `SafeAppend(s []int, v int) []int` returns a slice with the elements of `s` followed by `v`, but **never writes into `s`'s backing array**, even when `s` has spare capacity. Two calls on the same `s` must not interfere with each other.
- `Clone(s []int) []int` returns an independent copy with the same elements. A `nil` input gives `nil`; an empty non-nil input gives an empty non-nil slice.
- `Window(s []int, from, to int) []int` returns `s[from:to]` (it shares elements with `s`, so writing to the window changes `s`), but with its capacity limited so that appending to the window can **not** overwrite elements of `s` after `to`.
- `RemoveAt(s []int, i int) []int` removes the element at index `i`, keeping the order of the rest, and returns the shorter slice. It works in place on `s`'s array (no new array). Assume `0 <= i < len(s)`.
