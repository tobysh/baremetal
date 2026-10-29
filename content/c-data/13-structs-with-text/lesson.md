---
title: Structs That Hold Text
summary: Put a char array inside a struct, fill it safely with a bounded copy, and search a table of records by name.
minutes: 25
difficulty: medium
tags: structs, strings
id: c-data/structs-with-text
mode: harness
---
Records usually have names. A struct can contain an array, including a `char` array that holds a string:

```c
typedef struct {
    char name[16];
    int qty;
    int price_cents;
} Item;
```

The struct owns all 16 bytes of `name`, so a struct containing text can be copied whole: `Item b = a;` copies the name too (this is the one case where `=` copies an array: inside a struct).

## The text must fit

`name[16]` holds at most **15 characters** plus the terminator. Copy text in with a loop that stops at the end of the source **or** when the buffer is full, then terminate:

```c
int i = 0;
while (src[i] != '\0' && i < 15) {
    dst[i] = src[i];
    i++;
}
dst[i] = '\0';
```

Skipping the limit lets long names overflow into the neighbouring fields and memory. This is a real source of bugs: always know how big the destination is. The loop is the one from [Copying and Comparing Strings](/learn/c-data/copy-compare), plus the bound.

## Finding a record by name

Compare names with your own `str_equal`-style loop (or `strcmp` from `<string.h>`; `strcmp(a, b) == 0` means equal). `==` on char arrays would compare addresses, not text. Searching an array of structs is the [searching lesson](/learn/c-data/searching) with a name test: loop, return the index at the first match, `-1` if none.

## Your turn

Write `item_make`, and some functions that use a table of items. You may use `<string.h>` here, except for `strcpy`/`strncpy` in `item_make`: write that copy loop yourself.
