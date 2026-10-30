---
title: Mixed Investigation
summary: A cart shows a wrong total, then a corrupted counter. Separate two faults, patch both, and name the root cause of the corruption with a diagnosis code.
minutes: 35
difficulty: hard
tags: debugging, memory-corruption, evidence, diagnosis
id: c-debugging/mixed-investigation
mode: harness
---
This is the finale of the course. Real bugs rarely come alone, and the loudest symptom is not always the first one. You will **diagnose**, **patch** and **explain**.

## The program

A tiny shopping cart holds up to 4 lines. `add_item(c, qty, price)` should store the quantity, count the line, and add `qty * price` to the running total. When the cart is full it should refuse the line and change nothing.

```c
#define CAP 4

typedef struct {
    int qty[CAP];
    int used;     // lines stored
    int total;    // sum of qty * price
} cart;
```

All fields are `int`, so `used` sits directly after `qty[3]` in memory, and `total` after `used`.

## The evidence

A tester adds five lines in a row, `(qty, price)` = (2,10), (3,5), (1,20), (4,2), (7,3), and records the counters after each call:

```text
call  line      used  total   expected used  expected total
 1    (2,10)      1     10          1            20
 2    (3,5)       2     15          2            35
 3    (1,20)      3     35          3            55
 4    (4,2)       4     37          4            63
 5    (7,3)       8     40          4            63
```

A memory dump of the struct (each cell is one 4-byte `int`, in memory order) after calls 4 and 5:

```text
after 4:  qty[0..3] = 2 3 1 4 | used = 4 | total = 37
after 5:  qty[0..3] = 2 3 1 4 | used = 8 | total = 40
```

## Reading it

Two things are wrong, and they are not the same thing:

- The `total` column is wrong from call 1, long before the cart is full. `10` where `20` was expected is not a corruption: the counter simply counts the wrong amount. That is one fault.
- Call 5 is different. The cart was full, yet `used` became `8`. The quantity of that call was `7`, and `7 + 1 = 8`. The line was stored at `qty[4]`, one past the array, which is exactly where `used` lives. The write stored the quantity in `used`, and the following increment made it `8`. That is a second fault, and it explains a value that the first one cannot.

Ask of each symptom what would have to be true for it to appear. A wrong cost formula cannot make `used` jump to the quantity of the rejected line.

## Your turn

1. **Patch** `add_item` so that it returns `0` when the line is stored and `-1` when the cart is full (and then changes nothing: no write to `qty`, no change to `used` or `total`), and so that `total` adds `qty * price`.
2. **Diagnose.** `diagnosis()` must return the enum value that names the **root cause of the corrupted `used` counter in call 5**. The enum is already in the file. Its other values are plausible-sounding explanations that the evidence rules out.

## A note on grading the explanation

A written justification cannot be graded by a machine. The tests cannot read your reasoning, so they stand in for it in two ways: the `diagnosis()` code is compared with the true cause, and counterexample tests (full carts, repeated rejected calls, zero and large values, a guard placed after the struct) fail any patch that merely hides the symptoms in the table. Write the reasoning down for yourself anyway, as above: if you cannot say why each row of the table looks the way it does, you have guessed.
