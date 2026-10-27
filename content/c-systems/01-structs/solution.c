#include <stdio.h>

typedef struct {
    int x;
    int y;
} Point;

typedef struct {
    Point top_left;
    Point bottom_right;
} Rect;

Point point_make(int x, int y) {
    Point p = {x, y};
    return p;
}

Point point_add(Point a, Point b) {
    Point p = {a.x + b.x, a.y + b.y};
    return p;
}

int point_equal(Point a, Point b) {
    return a.x == b.x && a.y == b.y;
}

void point_scale(Point *p, int k) {
    p->x *= k;
    p->y *= k;
}

int rect_area(Rect r) {
    int width = r.bottom_right.x - r.top_left.x;
    int height = r.bottom_right.y - r.top_left.y;
    return width * height;
}

int main(void) {
    Point a = point_make(3, 4);
    printf("a = (%d, %d)\n", a.x, a.y);
    return 0;
}
