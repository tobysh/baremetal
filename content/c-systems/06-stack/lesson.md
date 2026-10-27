---
title: Stacks and Matching Brackets
summary: Build a last-in, first-out stack on a plain array, then use it to check that every bracket in a piece of text is properly closed.
minutes: 25
difficulty: medium
tags: data-structures, strings
id: c-systems/stack
mode: harness
---
A **stack** is the simplest useful data structure, and you already know one: a pile of plates. You add a plate to the top, and you take a plate from the top. The last plate you put on is the first one you take off. This is called **LIFO**: last in, first out.

Only two operations touch the pile:

- **push** puts a value on top.
- **pop** removes and returns the top value.

Often there is also **peek**, which looks at the top without removing it.

## An array and a counter

You do not need anything fancy. An array holds the items, and a counter says how many are in use. The top of the stack is always at index `size - 1`.

```c
#define STACK_MAX 32

typedef struct {
    int items[STACK_MAX];
    int size;
} Stack;
```

Pushing writes into the next free slot and bumps the counter. Popping lowers the counter and reads the slot:

```c
items: [ 7 | 3 | 9 |   |   ]      size = 3
                  ^ top

push(5)  ->  [ 7 | 3 | 9 | 5 |   ]   size = 4
pop()    ->  gives 5, size = 3 again
```

The array has a fixed size, so a push on a full stack must **fail** rather than write past the end, and a pop on an empty stack must fail rather than read garbage. Return `0` for success and `-1` for failure.

## Why stacks matter

Every time one function calls another, your computer uses a stack (the "call stack") to remember where to come back to. Undo buttons, browser back buttons and expression evaluators are all stacks too.

## Matching brackets

Stacks solve a classic puzzle: are the brackets in `"{[()]}"` properly paired? Walk the text:

- On an opening bracket `(`, `[` or `{`, **push** it.
- On a closing bracket, **pop** the top. It must be the matching opener, otherwise the text is wrong. If the stack is empty, there was nothing to close, which is also wrong.
- Ignore every other character.
- At the end the stack must be empty, otherwise something was never closed.

`"(]"` fails because `]` finds `(` on top. `"(("` fails because two openers are left over. The most recently opened bracket must be the first to close, which is exactly last-in, first-out.

## Your turn

Implement the stack operations, then `is_balanced` on top of them.
