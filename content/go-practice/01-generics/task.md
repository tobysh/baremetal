Implement these generic helpers:

- `Map[T, U any](xs []T, f func(T) U) []U` returns a new slice with `f` applied to each element, in order. It never returns `nil`, even for empty input.
- `Filter[T any](xs []T, keep func(T) bool) []T` returns the elements for which `keep` is true, in order. It never returns `nil`.
- `Reduce[T, A any](xs []T, init A, f func(A, T) A) A` folds the slice from left to right, starting from `init`.
- `Max[T cmp.Ordered](xs []T) (T, bool)` returns the largest element and `true`, or the zero value and `false` for an empty slice.
- `Stack[T any]` with `Push(x T)`, `Pop() (T, bool)` (last in, first out; zero value and `false` when empty) and `Len() int`. The zero `Stack` must be ready to use.
