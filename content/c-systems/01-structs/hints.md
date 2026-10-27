`point_make` can build a `Point` with `Point p = {x, y};` and return it. `point_add` is the same idea with `a.x + b.x` and `a.y + b.y`.
===
`point_scale` receives a pointer, so use the arrow: `p->x *= k;`. Arguments passed by value are copies, which is why this one needs a pointer.
===
```c
int rect_area(Rect r) {
    int width  = r.bottom_right.x - r.top_left.x;
    int height = r.bottom_right.y - r.top_left.y;
    return width * height;
}
```
