`ShapeKind` and `Shape` are defined in the starter. Implement:

- `Shape make_circle(int r)`, `Shape make_rect(int w, int h)` and `Shape make_triangle(int base, int height)` return a `Shape` with the right `kind` and the right payload filled in.
- `double shape_area(const Shape *s)` returns the area: circle `3.14159 * r * r`, rectangle `w * h`, triangle `base * height / 2.0`. Return `0.0` for a kind you do not recognise.
- `const char *kind_name(ShapeKind k)` returns `"circle"`, `"rectangle"` or `"triangle"`, and `"unknown"` for anything else (the tests pass a number that is not in the enum).
- `int count_kind(const Shape *shapes, int n, ShapeKind k)` returns how many of the `n` shapes have kind `k`. With `n` of `0` the answer is `0`.
