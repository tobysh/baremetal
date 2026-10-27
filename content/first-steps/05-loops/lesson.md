---
title: Loops
summary: Repeat work with while and for loops, and avoid the classic off-by-one mistake.
minutes: 20
difficulty: medium
tags: loops
id: first-steps/loops
mode: harness
---
A **loop** repeats a block of code. Computers are great at doing the same thing a million times without getting bored.

## while

```c
int count = 1;
while (count <= 3) {
    printf("%d\n", count);
    count = count + 1;
}
// prints 1, 2, 3
```

`while` keeps going as long as its condition is true. If the condition never becomes false — say you forget `count = count + 1` — the loop runs forever. (Our runner stops programs after 5 seconds, so you can't break anything.)

## for

Counting loops are so common that C has a compact form:

```c
for (int i = 1; i <= 3; i++) {
    printf("%d\n", i);
}
```

The three parts inside the parentheses are: **start** (`int i = 1`), **keep going while** (`i <= 3`), and **after each round** (`i++`, short for `i = i + 1`).

## Accumulating a result

A very common pattern is a variable that collects a result as the loop goes:

```c
int total = 0;
for (int i = 1; i <= 4; i++) {
    total += i;      // short for total = total + i
}
// total is 1 + 2 + 3 + 4 = 10
```

## Off-by-one errors

Should it be `<` or `<=`? Start at 0 or 1? Getting these wrong by one is so common it has a name. When in doubt, trace through the loop by hand with a tiny input like 1 or 2.
