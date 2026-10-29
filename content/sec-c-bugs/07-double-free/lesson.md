---
title: Make release repeatable
summary: Clear an owning pointer after release so repeated cleanup cannot free the same allocation twice.
minutes: 25
difficulty: easy
tags: security, defensive-programming
id: sec-c-bugs/double-free
mode: harness
---
Cleanup code is often reached through several paths. If two branches both call `free(p)`, the second call uses an address whose allocation is already gone. That can corrupt allocator state or terminate the process. A defensive cleanup helper can make repeated release through the same owner harmless.

C passes pointers by value. Setting a local parameter to NULL does not change the caller's pointer. Accept `void **owner` so the helper can replace the pointer stored by the caller. Free its value once, then store NULL. `free(NULL)` is defined to do nothing, so the second call becomes a safe no-op. Also accept a NULL owner address.

This pattern only protects the one owning slot. Copies of the address elsewhere remain dangling; it does not implement reference counting or make shared ownership safe. Prefer one clear owner and borrowed pointers whose lifetime stays within that owner. In larger structs, release each owned field then reset its length/capacity metadata. A destructor should leave an object in a documented state that can be inspected and cleaned again.

## The unsafe pattern

```c
void cleanup(void *p) {
    free(p);
    p = NULL; /* only changes a local copy */
}
```

The caller still holds the freed address. Repeating cleanup may abort or corrupt allocator state; clear the actual owning slot.
