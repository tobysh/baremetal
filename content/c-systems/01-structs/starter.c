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
    Point p = {0, 0};
    return p;
}

Point point_add(Point a, Point b) {
    Point p = {0, 0};
    return p;
}

int point_equal(Point a, Point b) {
    return 0;
}

void point_scale(Point *p, int k) {
}

int rect_area(Rect r) {
    return 0;
}

// Optional: try them with Run.
int main(void) {
    Point a = point_make(3, 4);
    printf("a = (%d, %d)\n", a.x, a.y);
    return 0;
}
