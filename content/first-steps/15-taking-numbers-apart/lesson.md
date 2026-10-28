---
title: Taking numbers apart
summary: Use % 10 and / 10 in a loop to visit every digit of a number, then count, add and reverse them.
minutes: 20
difficulty: medium
tags: loops, arithmetic
id: first-steps/taking-numbers-apart
mode: stdout
---
This lesson puts together three things you already know: [division and remainder](/learn/first-steps/division-and-remainder), [loops](/learn/first-steps/loops) and [reading input](/learn/first-steps/reading-input).

## Peeling off digits

For any whole number:

- `n % 10` is its **last digit**.
- `n / 10` is the number **without** its last digit.

So `4821 % 10` is 1 and `4821 / 10` is 482. Repeat until nothing is left:

```c
int n = 4821;
while (n > 0) {
    int digit = n % 10;     // 1, then 2, then 8, then 4
    printf("%d\n", digit);
    n = n / 10;             // 482, 48, 4, 0
}
```

The loop stops when `n` reaches 0, because `4 / 10` is 0. The digits come out **right to left**.

## Building a result as you go

Inside the loop you can collect something from each digit:

```c
int sum = 0;
int count = 0;
while (n > 0) {
    sum += n % 10;      // add the digit
    count++;            // count it
    n /= 10;
}
```

To **reverse** a number, push each digit onto the end of a new number: multiply what you have by 10 and add the digit.

```c
reversed = reversed * 10 + digit;
```

For 4821 that builds 1, then 12, then 128, then 1284.

## Remember the original

The loop destroys `n`. If you still need the original number later, copy it first (`int rest = n;`) and take the loop apart on the copy.
