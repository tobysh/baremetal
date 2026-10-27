`Point` and `Rect` are defined in the starter. Implement:

- `Point point_make(int x, int y)` returns a point with those coordinates.
- `Point point_add(Point a, Point b)` returns a new point whose coordinates are the sums.
- `int point_equal(Point a, Point b)` returns `1` if both coordinates match, otherwise `0`.
- `void point_scale(Point *p, int k)` multiplies both coordinates of the point `p` points to by `k`, changing it in place.
- `int rect_area(Rect r)` returns width times height. `top_left` always has the smaller `x` and the smaller `y`.
