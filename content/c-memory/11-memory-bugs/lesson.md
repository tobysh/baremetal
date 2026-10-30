---
title: "Memory Bugs: Leaks, Use After Free, Double Free"
summary: Learn to recognise the three classic heap bugs by reading code, and fix a buggy program so every block is freed exactly once and never touched afterwards.
minutes: 30
difficulty: medium
tags: heap, malloc, debugging
id: c-memory/memory-bugs
mode: harness
---
The heap gives you freedom and one strict rule: **every block you allocate is freed exactly once, and not used afterwards.** Break it and C will often not complain at all. The program may seem to work, then fail in another place days later. Learning to spot these bugs by reading is one of the most useful C skills.

## 1. Memory leak

A block that is never freed. The program lost the pointer, so the memory is stuck until the program exits.

```c
char *shout(const char *s) {
    char *tmp = malloc(strlen(s) + 1);   // scratch copy
    strcpy(tmp, s);
    /* ... upper-case it, build the result ... */
    return result;                       // tmp is never freed: leak
}
```

Every `return` path must free everything the function allocated and is not handing back.

## 2. Use after free

Using a pointer after `free` has given the block back. The block may be reused by another allocation, so you read or corrupt someone else's data.

```c
free(p);
total += p[0];      // WRONG: p is dangling
```

A **dangling pointer** still holds the old address, which looks valid but is not. The fix is nearly always to reorder: finish using the data, then free. Setting `p = NULL` right after `free(p)` turns a silent bug into an immediate crash, which is far easier to find.

## 3. Double free

Calling `free` twice on the same block corrupts the allocator's bookkeeping and is a classic security hole.

```c
free(person->name);
free(person->name);     // WRONG: second free of the same block
```

This often happens when two code paths both think they own the block, or when a cleanup function is copy-pasted. Keep a clear rule about who owns each block.

## Finding them

In real projects tools help: `valgrind` and the compiler option `-fsanitize=address` report these bugs at run time. Here, the hidden tests watch every `malloc` and `free` your code makes. They count blocks still alive after your function, they notice a second `free` of the same block, and they scribble over freed memory so that reading it gives obvious nonsense.

## Your turn

The starter has three small functions. Each has exactly one memory bug, one of each kind. Read each carefully, work out which rule it breaks, and fix it without changing what it computes.
