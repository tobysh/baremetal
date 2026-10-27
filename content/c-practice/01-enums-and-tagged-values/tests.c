static int near(double a, double b) { return a - b < 0.001 && b - a < 0.001; }

int main(void) {
    Shape c = make_circle(2);
    Shape r = make_rect(3, 4);
    Shape t = make_triangle(5, 3);
    CHECK("make_circle sets the tag", c.kind == SHAPE_CIRCLE, "kind = %d", (int)c.kind);
    CHECK("make_rect sets the tag", r.kind == SHAPE_RECT, "kind = %d", (int)r.kind);
    CHECK("make_triangle sets the tag", t.kind == SHAPE_TRIANGLE, "kind = %d", (int)t.kind);
    CHECK("make_rect stores width and height", r.u.rect.w == 3 && r.u.rect.h == 4, "w = %d, h = %d", r.u.rect.w, r.u.rect.h);
    CHECK("make_circle stores the radius", c.u.circle.r == 2, "r = %d", c.u.circle.r);
    CHECK("make_triangle stores base and height", t.u.tri.base == 5 && t.u.tri.height == 3, "base = %d, height = %d", t.u.tri.base, t.u.tri.height);

    CHECK("circle of radius 2 has area 12.56636", near(shape_area(&c), 12.56636), "got %f", shape_area(&c));
    CHECK("3 by 4 rectangle has area 12", near(shape_area(&r), 12.0), "got %f", shape_area(&r));
    CHECK("triangle 5 by 3 has area 7.5 (halves are kept)", near(shape_area(&t), 7.5), "got %f", shape_area(&t));
    Shape zero = make_rect(0, 9);
    CHECK("a rectangle with zero width has area 0", near(shape_area(&zero), 0.0), "got %f", shape_area(&zero));
    Shape bad = r;
    bad.kind = (ShapeKind)42;
    CHECK("an unknown kind has area 0", near(shape_area(&bad), 0.0), "got %f", shape_area(&bad));

    CHECK("kind_name circle", strcmp(kind_name(SHAPE_CIRCLE), "circle") == 0, "got \"%s\"", kind_name(SHAPE_CIRCLE));
    CHECK("kind_name rectangle", strcmp(kind_name(SHAPE_RECT), "rectangle") == 0, "got \"%s\"", kind_name(SHAPE_RECT));
    CHECK("kind_name triangle", strcmp(kind_name(SHAPE_TRIANGLE), "triangle") == 0, "got \"%s\"", kind_name(SHAPE_TRIANGLE));
    CHECK("kind_name of a number not in the enum is unknown", strcmp(kind_name((ShapeKind)42), "unknown") == 0, "got \"%s\"", kind_name((ShapeKind)42));

    Shape all[5];
    all[0] = make_rect(1, 1);
    all[1] = make_circle(1);
    all[2] = make_rect(2, 2);
    all[3] = make_triangle(2, 2);
    all[4] = make_rect(3, 3);
    CHECK("count_kind counts three rectangles", count_kind(all, 5, SHAPE_RECT) == 3, "got %d", count_kind(all, 5, SHAPE_RECT));
    CHECK("count_kind counts one circle", count_kind(all, 5, SHAPE_CIRCLE) == 1, "got %d", count_kind(all, 5, SHAPE_CIRCLE));
    CHECK("count_kind only looks at the first n shapes", count_kind(all, 2, SHAPE_RECT) == 1, "got %d", count_kind(all, 2, SHAPE_RECT));
    CHECK("count_kind with n = 0 is 0", count_kind(all, 0, SHAPE_RECT) == 0, "got %d", count_kind(all, 0, SHAPE_RECT));
    return 0;
}
