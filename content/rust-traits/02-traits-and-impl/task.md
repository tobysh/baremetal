The starter defines `trait Shape` (with `name`, `area`, `perimeter` and a ready-made default `describe`), and structs `Square { side: u32 }` and `Rectangle { width: u32, height: u32 }`.

- Implement `Shape` for `Square`: name `"square"`, area `side * side`, perimeter `4 * side`.
- Implement `Shape` for `Rectangle`: name `"rectangle"`, area `width * height`, perimeter `2 * (width + height)`.
- Write `fn total_area<T: Shape>(shapes: &[T]) -> u32` returning the sum of all the areas.

Do not override `describe`; the default should work once the other methods do.
