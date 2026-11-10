---
title: Arrays of Structs
summary: Keep a table of records in an array of structs, loop over it, update records in place, and sort it by one field.
minutes: 25
difficulty: medium
tags: structs, arrays, algorithms, design
id: c-data/arrays-of-structs
mode: stdout
revision: 2
---
One record is useful; a table of records is how programs hold real data: players, products, orders. Put structs in an array, just like `int`s:

```c
typedef struct {
    int id;
    int score;
} Entry;

Entry board[3] = {{1, 70}, {2, 95}, {3, 80}};

printf("%d\n", board[1].score);   // 95
board[2].score = 85;
```

`board[i]` is a whole `Entry`; `board[i].score` is one field of it. Everything from [Arrays and Indexing](/learn/c-data/arrays) applies, with `.field` added.

## Functions work on the original array

A function that takes `Entry e[]` and a length, like an `int` array, sees the **caller's** records, so it can change them: `e[i].score += 5;` is visible afterwards. (A single `Entry` passed by value is a copy, as in [Your First Struct](/learn/c-data/first-struct).) This is why an array of structs is the natural way to update many records at once without pointers.

## Sorting a table

To sort records by one field, you can use **insertion sort**: keep the front of the array sorted, and insert each next element into its place by shifting bigger ones along. Swapping works on whole structs too: `Entry tmp = e[i]; e[i] = e[j]; e[j] = tmp;` copies all fields.

```c
for (int i = 1; i < n; i++) {
    Entry current = e[i];
    int j = i - 1;
    while (j >= 0 && e[j].score < current.score) {   // "<" means highest first
        e[j + 1] = e[j];
        j--;
    }
    e[j + 1] = current;
}
```

Insertion sort is **stable**: records with equal scores stay in their original order, because we only move a record past strictly smaller ones.

## Design it yourself

This lesson used to give you five function signatures. Now you choose them. The starter is empty: define your own struct, write the functions you want and a `main` that reads the input and prints the report. Follow the design recipe:

1. **Understand the problem.** Read the task twice. What is the input, and what must each output line say?
2. **Write examples.** Work out the full output by hand for the example, for a single entry, and for an empty scoreboard.
3. **Choose signatures.** The five jobs (find the best, add up, count the passes, add a bonus, sort) each become a function over an array of structs and its length. Decide each one's parameters and return type before writing bodies.
4. **Plan.** The patterns here are an **accumulator** (the total), a **counter** with a condition (the passes), a **search** (the best entry), and a stable sort for the last step.
5. **Code.** One function at a time, trying each with a small `main` before moving on.
6. **Test.** Press Run with your own inputs. When an answer is wrong, say which function you suspect and print what it receives and returns.
