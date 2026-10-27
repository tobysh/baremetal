The starter defines the `Shape` trait (with a default `summary` method) and the types `Square(f64)` (side), `Rect { w, h }` and `Circle { r }`.

- Implement `Shape` for all three. `name` returns `"square"`, `"rect"` or `"circle"`. Do not override `summary`.
- `fn total_area<T: Shape>(items: &[T]) -> f64` returns the sum of the areas, or `0.0` for an empty slice.
- `fn describe_largest(items: &[Box<dyn Shape>]) -> Option<String>` returns the `summary()` of the shape with the greatest area, or `None` if the slice is empty.
- `fn report(items: &[Box<dyn Shape>]) -> String` returns every shape's `summary()` joined with `"; "`, in order. An empty slice gives an empty string.
