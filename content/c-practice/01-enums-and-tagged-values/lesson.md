---
title: Enums and Tagged Values
summary: Name a set of choices with an enum, then use a union plus a tag to hold one of several different shapes of data in the same space.
minutes: 20
difficulty: easy
tags: enums, structs
id: c-practice/enums-and-tagged-values
mode: harness
---
Sometimes a value can be one of a few *kinds*. A shape on a map might be a circle, a rectangle or a triangle. A message might be a number or a word. C gives you two small tools for this, and they work best together.

## Enums: names for choices

An **enum** is a list of names. Behind the scenes each name is just an integer, starting at `0` and counting up, but your code reads like plain English:

```c
typedef enum {
    SHAPE_CIRCLE,      // 0
    SHAPE_RECT,        // 1
    SHAPE_TRIANGLE     // 2
} ShapeKind;
```

Now `SHAPE_RECT` is much clearer than the mystery number `1`. A `switch` on an enum reads nicely too:

```c
switch (kind) {
    case SHAPE_CIRCLE:   return "circle";
    case SHAPE_RECT:     return "rectangle";
    case SHAPE_TRIANGLE: return "triangle";
}
return "unknown";      // for a number that matches no name
```

Always keep a fallback after the `switch`: an enum is only an integer, so someone can pass `42` and C will not stop them.

## Unions: one box, several possible contents

A **union** looks like a struct, but its members *share the same memory*. It is as big as its biggest member, and only one member is meaningful at a time.

```c
union Number {
    int    whole;
    double fraction;
};
```

If you store into `whole` and then read `fraction`, you get nonsense. The union has no idea which one you last used. So we keep a note of it ourselves.

## The tagged union

Pair an enum (the **tag**) with a union (the **payload**) inside a struct:

```c
typedef struct {
    ShapeKind kind;            // the tag: which member is valid
    union {
        struct { int r; }       circle;
        struct { int w, h; }    rect;
        struct { int base, height; } tri;
    } u;
} Shape;
```

The rule is simple: **look at the tag first, then touch only the matching member.**

```c
if (s->kind == SHAPE_RECT) {
    area = s->u.rect.w * s->u.rect.h;
}
```

This pattern is everywhere: interpreters, network messages, and the `enum` types in Rust that you may meet later are the same idea with the safety checks built in.

## Your turn

The starter defines `Shape`. Write small constructor functions, an `area` function that checks the tag, a function that turns a kind into its name, and a counter.
