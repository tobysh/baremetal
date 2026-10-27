A constructor can build the struct in a local variable and return it: `Shape s; s.kind = SHAPE_RECT; s.u.rect.w = w; s.u.rect.h = h; return s;`
===
For `shape_area` and `kind_name`, use a `switch` on the kind. Each `case` only touches its own union member. Put a `return` after the `switch` for the unknown case.
===
```c
double shape_area(const Shape *s) {
    switch (s->kind) {
        case SHAPE_CIRCLE:   return 3.14159 * s->u.circle.r * s->u.circle.r;
        case SHAPE_RECT:     return (double)s->u.rect.w * s->u.rect.h;
        case SHAPE_TRIANGLE: return s->u.tri.base * s->u.tri.height / 2.0;
    }
    return 0.0;
}
```
