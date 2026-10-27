The starter defines:

```rust
enum Shape {
    Circle(f64),             // radius
    Rectangle(f64, f64),     // width, height
    Triangle(f64, f64, f64), // three side lengths
}
```

Write two functions that use `match`:

- `fn area(s: &Shape) -> f64` returns the area. Use `std::f64::consts::PI` for circles and Heron's formula for triangles.
- `fn perimeter(s: &Shape) -> f64` returns the perimeter (for a circle, the circumference).
