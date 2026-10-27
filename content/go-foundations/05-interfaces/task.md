`Shape` has `Area() float64` and `Perimeter() float64`. Implement both methods for `Rect` (fields `W`, `H`) and `Circle` (field `R`, use `math.Pi`), then write:

- `Largest(shapes []Shape) Shape` returns the shape with the biggest area, or `nil` for an empty slice.
- `Describe(s Shape) string` returns `"rect WxH"` for a `Rect`, `"circle r=R"` for a `Circle`, and `"unknown"` for anything else. Print the numbers with `%v`, e.g. `"rect 3x4"`, `"circle r=2.5"`.
