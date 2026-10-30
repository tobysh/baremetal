---
title: Stopping and skipping
summary: Use continue to skip one pass of a loop and break to leave it early.
minutes: 15
difficulty: medium
tags: loops, control-flow
id: first-steps/break-and-continue
mode: stdout
---
A loop normally runs until its condition turns false. Two keywords let you change that from the inside.

## continue: skip this pass

`continue` jumps straight to the next pass of the loop. The rest of the body is skipped.

```c
for (int i = 1; i <= 5; i++) {
    if (i == 3) {
        continue;
    }
    printf("%d\n", i);
}
// prints 1, 2, 4, 5
```

In a `for` loop, `i++` still runs before the next pass, so `continue` never gets you stuck.

## break: leave the loop

`break` ends the loop immediately and the program carries on after the closing brace.

```c
for (int i = 1; i <= 100; i++) {
    if (i * i > 50) {
        break;
    }
    printf("%d\n", i);
}
// prints 1 to 7, then stops
```

Without `break`, this loop would run all 100 passes.

## Using both

Checks at the top of the body keep the rest of the code simple: handle the cases to skip or stop on first, then do the normal work.

`break` only leaves the **innermost** loop it sits in. In nested loops, the outer loop keeps going.
