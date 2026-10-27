#include <stdio.h>

typedef enum {
    SHAPE_CIRCLE,
    SHAPE_RECT,
    SHAPE_TRIANGLE
} ShapeKind;

typedef struct {
    ShapeKind kind;
    union {
        struct { int r; } circle;
        struct { int w, h; } rect;
        struct { int base, height; } tri;
    } u;
} Shape;

Shape make_circle(int r) {
    Shape s = {0};
    s.kind = SHAPE_CIRCLE;
    s.u.circle.r = r;
    return s;
}

Shape make_rect(int w, int h) {
    Shape s = {0};
    s.kind = SHAPE_RECT;
    s.u.rect.w = w;
    s.u.rect.h = h;
    return s;
}

Shape make_triangle(int base, int height) {
    Shape s = {0};
    s.kind = SHAPE_TRIANGLE;
    s.u.tri.base = base;
    s.u.tri.height = height;
    return s;
}

double shape_area(const Shape *s) {
    switch (s->kind) {
        case SHAPE_CIRCLE:   return 3.14159 * s->u.circle.r * s->u.circle.r;
        case SHAPE_RECT:     return (double)s->u.rect.w * s->u.rect.h;
        case SHAPE_TRIANGLE: return s->u.tri.base * s->u.tri.height / 2.0;
    }
    return 0.0;
}

const char *kind_name(ShapeKind k) {
    switch (k) {
        case SHAPE_CIRCLE:   return "circle";
        case SHAPE_RECT:     return "rectangle";
        case SHAPE_TRIANGLE: return "triangle";
    }
    return "unknown";
}

int count_kind(const Shape *shapes, int n, ShapeKind k) {
    int count = 0;
    for (int i = 0; i < n; i++) {
        if (shapes[i].kind == k) {
            count++;
        }
    }
    return count;
}

int main(void) {
    Shape r = make_rect(3, 4);
    printf("%s: %.2f\n", kind_name(r.kind), shape_area(&r));
    return 0;
}
