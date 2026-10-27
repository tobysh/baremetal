---
title: Structs and typedef
summary: Bundle related values into one struct, give it a friendly name with typedef, pass it around by value or by pointer, and nest one struct inside another.
minutes: 20
difficulty: easy
tags: structs
id: c-systems/structs
mode: harness
---
So far every variable held one value. But real things have several parts. A point on a map has an `x` and a `y`. A knight has a name, a level and some hit points. Carrying these around as separate variables gets messy fast.

A **struct** is a box with labelled compartments. You define the shape of the box once, then make as many boxes as you like.

```c
struct Point {
    int x;
    int y;
};

struct Point p = {3, 4};
printf("%d, %d\n", p.x, p.y);   // 3, 4
p.x = 10;                       // change one compartment
```

The dot `.` means "the compartment called ...". Writing `struct Point` every time is wordy, so C lets you give the type a shorter name with `typedef`:

```c
typedef struct {
    int x;
    int y;
} Point;

Point p = {3, 4};
```

Now `Point` works like `int` does.

## Structs are copied

Pass a struct to a function and the function gets its own **copy**, just like an `int`. Return one and the caller gets a copy back.

```c
Point point_make(int x, int y) {
    Point p = {x, y};
    return p;
}
```

Because it is a copy, changing it inside a function does not change the caller's struct. To change the original, pass its address, exactly as you did in the pointers lesson:

```c
void move_right(Point *p) {
    (*p).x += 1;     // follow the pointer, then pick the compartment
    p->x += 1;       // the same thing, with a nicer spelling
}

move_right(&p);
```

The arrow `->` is shorthand for "follow this pointer, then pick this compartment". You will see it everywhere in C.

## Structs inside structs

A compartment can itself hold a struct:

```c
typedef struct {
    Point top_left;
    Point bottom_right;
} Rect;

Rect r = {{0, 0}, {4, 3}};
int width = r.bottom_right.x - r.top_left.x;   // 4
```

Chain the dots to dig in: `r.bottom_right.x`.

## Your turn

The `Point` and `Rect` types are already written for you. Fill in the functions that make, add, compare and scale points, and measure a rectangle.
