---
title: Crash Trace
summary: A crash report names a faulting address and a call chain. Work out what it proves and what it only suggests, then fix the program with a minimal change.
minutes: 20
difficulty: easy
tags: debugging, null-pointer, evidence
id: c-debugging/crash-trace
mode: harness
---
A crash is the cheapest evidence you will ever get: the machine tells you exactly where it gave up. Your job is to read it without over-reading it.

## The report

A price calculator crashes every time it is asked about the order `"pen", "ink", "stapler"`. The shop's log shows:

```text
fault: SIGSEGV, read of address 0x0000000000000008
  #0  price_of      shop.c:21   return it->cents;
  #1  order_total   shop.c:30   total += price_of(list, names[i]);
  #2  main          shop.c:52   printf("%d\n", order_total(list, order, 3));
```

The relevant types:

```c
typedef struct item {
    const char  *name;    // offset 0
    int          cents;   // offset 8
    struct item *next;    // offset 16
} item;
```

## Reading the trace

Frame `#0` is where the crash happened; each frame below it is the caller.

**What the trace proves**

- The faulting instruction is the read of `it->cents` on line 21, inside `price_of`.
- The address was `0x8`. `cents` sits at offset 8, so the base pointer `it` was **NULL**: `NULL + 8 = 0x8`. A small address like this is the signature of a NULL pointer used as a struct.
- `price_of` was called from `order_total`, which was called from `main`. The caller chain is real.

**What the trace only suggests**

- That `find_item` returned NULL because `"stapler"` is not in the list. Likely, but the trace does not show the list or the names. The list could also be empty, or `list` itself could be NULL.
- That `order_total` is the function to blame. It passed a name that has no price, but whether a missing item is a *caller* bug or something `price_of` must tolerate is a design decision. The trace cannot make it.
- That this is the only bad name. The crash stops the program at the first one; later names were never examined.

A good habit is to write the two lists down before touching code. Anything in the second list needs a test or a print statement before you rely on it.

## The fix

`price_of` should return `-1` when the item cannot be found, whatever the reason (including a NULL list). `order_total` then adds up only the items that exist and silently skips unknown names.

Be careful: once `price_of` can return `-1`, adding its result blindly would subtract a cent from the total. A fix to the crash that quietly gives wrong totals is a new bug.

Keep the change small. You do not need to touch `find_item`, the struct, or `main`.
