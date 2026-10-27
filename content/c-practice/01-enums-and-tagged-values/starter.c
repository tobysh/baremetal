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
    return s;
}

Shape make_rect(int w, int h) {
    Shape s = {0};
    return s;
}

Shape make_triangle(int base, int height) {
    Shape s = {0};
    return s;
}

double shape_area(const Shape *s) {
    return 0.0;
}

const char *kind_name(ShapeKind k) {
    return "";
}

int count_kind(const Shape *shapes, int n, ShapeKind k) {
    return 0;
}

// Optional: try them with Run.
int main(void) {
    Shape r = make_rect(3, 4);
    printf("%s: %.2f\n", kind_name(r.kind), shape_area(&r));
    return 0;
}
