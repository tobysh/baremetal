---
title: Choosing with switch
summary: Pick one of many branches by the value of an integer or character with switch.
minutes: 15
difficulty: medium
tags: decisions, control-flow
id: first-steps/switch
mode: stdout
---
A chain of `else if` tests that all compare the same value gets long. `switch` is made for that case.

```c
int grade = 2;
switch (grade) {
    case 1:
        printf("first\n");
        break;
    case 2:
        printf("second\n");
        break;
    default:
        printf("other\n");
        break;
}
```

C jumps to the `case` whose value matches and runs from there. `default` runs when nothing matches.

## Why `break` matters

A `switch` does **not** stop at the end of a case. It keeps running into the next one until it meets a `break`. Forgetting `break` is the classic switch bug:

```c
switch (grade) {
    case 1: printf("first\n");     // no break
    case 2: printf("second\n");    // grade 1 prints both lines
}
```

## Sharing a branch

Leaving out `break` on purpose lets several values share the same code:

```c
switch (c) {
    case 'a':
    case 'e':
    case 'i':
    case 'o':
    case 'u':
        printf("vowel\n");
        break;
    default:
        printf("not a vowel\n");
        break;
}
```

`switch` works on integers and characters (not decimals or text), and each `case` must be a constant such as `3` or `'a'`, not a variable.
