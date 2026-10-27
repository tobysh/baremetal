---
title: Recursion and Backtracking
summary: Solve puzzles by trying a choice, exploring what follows, and undoing it. Count subsets that add up and the ways to place queens safely.
minutes: 45
difficulty: hard
tags: recursion, algorithms
id: c-practice/recursion-and-backtracking
mode: harness
---
A **recursive** function calls itself on a smaller version of the problem. Two ingredients make it work:

1. a **base case**, where the answer is obvious and no more calls are made;
2. a **step** that moves closer to the base case.

```c
int sum_to(int n) {
    if (n == 0) return 0;         // base case
    return n + sum_to(n - 1);     // smaller problem
}
```

If you forget the base case, the calls never stop and the program crashes when the call stack runs out of room.

## Digits, warm-up

The last digit of `n` is `n % 10`, and the rest is `n / 10`. So the digit sum of `472` is `2 + digit_sum(47)`, and the base case is a number below `10`, whose digit sum is itself. Practise that first.

## Backtracking: try, explore, undo

Some problems have many possible choices at each step, and you must try them all. **Backtracking** is recursion with a pattern:

1. **Choose** one option.
2. **Explore** by recursing to make the remaining choices.
3. **Undo** the choice, so the next option starts from a clean slate.

### Subsets that add up

For each number you make one decision: *take it* or *leave it*. To count the ways of reaching a target sum from the numbers `a[index ...]`:

```c
int count(const int *a, int n, int index, int remaining) {
    if (index == n) return remaining == 0 ? 1 : 0;   // all decided
    int taken = count(a, n, index + 1, remaining - a[index]);
    int left  = count(a, n, index + 1, remaining);
    return taken + left;
}
```

Here the "undo" is free because `remaining` is passed by value, so each call has its own copy. Note the empty subset (take nothing) counts as a subset with sum `0`.

### Queens that cannot capture each other

A chess queen attacks along her row, column and both diagonals. Can you place `n` queens on an `n` by `n` board so that none attacks another? Count the ways.

Since every row needs exactly one queen, decide row by row: *which column does the queen in this row go in?* Store the answers in an array `cols[row] = column`. A new queen at `(row, c)` is safe if for every earlier row `r`:

- `cols[r] != c` (not the same column), and
- `cols[r] - c` is not `r - row` or `row - r` in size (not the same diagonal): the difference in columns must differ from the difference in rows.

```c
static int place(int n, int row, int *cols) {
    if (row == n) return 1;            // every queen placed: one solution
    int total = 0;
    for (int c = 0; c < n; c++) {
        if (safe(cols, row, c)) {
            cols[row] = c;             // choose
            total += place(n, row + 1, cols);   // explore
            // undo: the next loop turn overwrites cols[row], so nothing more to do
        }
    }
    return total;
}
```

The number of solutions is famous: 1, 0, 0, 2, 10, 4, 40, 92 for boards of size 1 to 8. If your count matches those, your search is right.

## Your turn

Write `sum_digits`, `count_subsets` and `count_queens`. Use small helper functions if you like (mark them `static`).
