---
title: Keep an owned copy alive
summary: Repair a lifetime error by copying data while the owner is alive and returning the new owner.
minutes: 25
difficulty: medium
tags: security, defensive-programming
id: sec-c-bugs/use-after-free
mode: harness
---
A pointer is an address, not ownership. Returning a pointer after freeing its allocation leaves the caller holding a dangling pointer. Reading it is undefined behaviour even if a quick run still prints the old text. The allocator is free to reuse the bytes immediately.

Our function must return a fresh owned copy of a label. Compute its size while the input is alive, allocate a separate buffer, copy the terminator as well as the characters, and return that allocation without freeing it. The caller now owns the result and will call `free` once. A NULL input has no label and returns NULL.

Do not return the input pointer: it might refer to a stack array or read-only string literal, and the caller is promised ownership. Also do not return a local array, whose lifetime ends at return. Writing an ownership sentence beside each public pointer parameter and return value often catches these mistakes before tests do. Allocation failure must propagate as NULL; callers cannot safely use a buffer that was never allocated. These rules apply equally to early-return and error paths.

## The unsafe pattern

```c
char *p = malloc(n + 1);
/* populate p */
free(p);
return p; /* dangling */
```

A later allocation can reuse the same bytes. A caller reading this pointer may disclose unrelated data or act on corrupted state. The returned owner must stay live.
