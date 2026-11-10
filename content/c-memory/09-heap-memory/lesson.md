---
title: Heap Memory
summary: Stack versus heap, and how to borrow memory with malloc, check for NULL, size it with sizeof, and give it back with free so you do not leak.
minutes: 25
difficulty: hard
tags: heap, malloc
id: c-memory/heap-memory
mode: harness
---
So far every variable you made lived on the **stack**. The stack is automatic memory: when a function is called its local variables appear, and when the function returns they are wiped away, like a whiteboard erased at the end of the meeting.

That is a problem if you want to build something inside a function and hand it back:

```c
int *broken(void) {
    int arr[3] = {1, 2, 3};
    return arr;          // WRONG: arr is erased when we return
}
```

The caller receives a pointer to a whiteboard that has already been wiped. Using it is undefined behaviour.

## What's in memory: a returned stack pointer

Here is the broken function in action (addresses made up). The array lives in `broken`'s frame, so it exists only until `broken` returns:

```text
while broken runs    broken's frame   arr  [1 2 3]  at 0x7000
                     main's frame     p    (not set yet)

after the return     main's frame     p    0x7000   <- points at space that is free again
                     (the next call will reuse that space for something else)
```

`p` still holds an address, but nothing guarantees what is there. The compiler notices this one. With the site's usual flags you get a warning even before you run:

```text
warning: function returns address of local variable [-Wreturn-local-addr]
```

and this program then stops with `runtime error: load of null pointer of type 'int'`. On other compilers or settings you might see `1`, a garbage number, or a crash, which is exactly what **undefined behaviour** means. This is also what **lifetime** is about: a stack variable lives until its function returns; heap memory lives until you call `free`.

## The heap

The **heap** is a big pool of memory you can borrow from and keep for as long as you like. You ask for it with `malloc` and give it back with `free`. Both live in `<stdlib.h>`.

```c
int *p = malloc(3 * sizeof(int));   // ask for room for 3 ints
```

`malloc(n)` reserves `n` bytes and returns a pointer to the first one. It does not erase the memory first, so the bytes contain leftover garbage until you write to them.

Now the array is not in `make`'s frame at all. The frame holds only the pointer, and the three ints sit on the heap:

```text
stack                                   heap
make's frame: p  0x9000  ----------->   0x9000: [ 0 | 1 | 2 ]    (12 bytes from malloc)

after make returns: main's frame:  q  0x9000  ----------->  the same block, still alive
```

The `make` frame is erased on return, but the block is not. The pointer was copied to `main`, so the block survives until someone calls `free`.

## sizeof

`sizeof(int)` is the number of bytes an `int` occupies (usually 4). To get room for `n` ints, ask for `n * sizeof(int)` bytes. A tidy trick that survives type changes is `sizeof *p`:

```c
int *p = malloc(n * sizeof *p);
```

## Always check for NULL

If the system cannot give you the memory, `malloc` returns `NULL`, the "pointer to nothing". Using it would crash, so always check:

```c
int *p = malloc(n * sizeof *p);
if (p == NULL) {
    return NULL;      // report failure to the caller
}
for (int i = 0; i < n; i++) {
    p[i] = i;
}
```

## free, and memory leaks

When you are finished, hand the memory back:

```c
free(p);
```

Heap memory is never reclaimed automatically. If you lose the pointer without calling `free`, that block is stuck, unusable, until the program ends. That is a **memory leak**. Long-running programs that leak eventually run out of memory.

The ownership rule: whoever receives a heap pointer is responsible for freeing it exactly once. A function that returns `malloc`'d memory is passing that responsibility to its caller.

## The three deadly mistakes

- **Use after free:** reading or writing through a pointer after `free(p)`.
- **Double free:** calling `free` twice on the same pointer.
- **Buffer overflow:** writing more than you asked for. A string needs `strlen(s) + 1` bytes: one for each character plus the `'\0'`. Forgetting the `+ 1` is probably the most common C bug there is.

A habit that helps: after `free(p);` set `p = NULL;`. Freeing NULL is always safe and does nothing.

## Your turn

Build an array on the heap and return it, then write your own version of `strdup` (copy a string into fresh heap memory). The tests free whatever you return.
