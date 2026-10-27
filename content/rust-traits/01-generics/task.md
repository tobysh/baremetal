Complete these in the starter:

- `fn largest<T: PartialOrd + Copy>(items: &[T]) -> Option<T>` returns the largest item, or `None` for an empty slice. It must work for `i32`, `f64` and `char`.
- `impl<T> Pair<T>` with `fn swap(self) -> Pair<T>`, which returns a new pair with `first` and `second` exchanged.
