int main(void) {
    Point p = point_make(3, 4);
    CHECK("point_make(3, 4) has x = 3 and y = 4", p.x == 3 && p.y == 4, "got (%d, %d)", p.x, p.y);

    Point n = point_make(-7, 0);
    CHECK("point_make keeps negative coordinates", n.x == -7 && n.y == 0, "got (%d, %d)", n.x, n.y);

    Point s = point_add(point_make(1, 2), point_make(10, 20));
    CHECK("point_add adds each coordinate", s.x == 11 && s.y == 22, "got (%d, %d)", s.x, s.y);

    Point a = point_make(5, 5);
    Point b = point_make(-5, 1);
    Point c = point_add(a, b);
    CHECK("point_add does not change its inputs", a.x == 5 && a.y == 5 && b.x == -5 && b.y == 1 && c.x == 0 && c.y == 6, "a = (%d, %d), c = (%d, %d)", a.x, a.y, c.x, c.y);

    CHECK("point_equal is 1 for matching points", point_equal(point_make(2, 3), point_make(2, 3)) == 1, "expected 1");
    CHECK("point_equal is 0 when only x differs", point_equal(point_make(2, 3), point_make(9, 3)) == 0, "expected 0");
    CHECK("point_equal is 0 when only y differs", point_equal(point_make(2, 3), point_make(2, 9)) == 0, "expected 0");

    Point q = point_make(2, -3);
    point_scale(&q, 4);
    CHECK("point_scale changes the point in place", q.x == 8 && q.y == -12, "got (%d, %d)", q.x, q.y);
    point_scale(&q, 0);
    CHECK("point_scale by 0 gives the origin", q.x == 0 && q.y == 0, "got (%d, %d)", q.x, q.y);

    Rect r = {{0, 0}, {4, 3}};
    CHECK("rect_area of 4 by 3 is 12", rect_area(r) == 12, "got %d", rect_area(r));
    Rect r2 = {{2, 5}, {3, 6}};
    CHECK("rect_area works away from the origin", rect_area(r2) == 1, "got %d", rect_area(r2));
    Rect flat = {{1, 1}, {9, 1}};
    CHECK("a flat rectangle has area 0", rect_area(flat) == 0, "got %d", rect_area(flat));
    return 0;
}
