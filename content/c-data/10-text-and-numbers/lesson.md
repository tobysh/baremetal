---
title: Text to Numbers and Back
summary: Convert digit characters to an int the way atoi does, and build the decimal text of an int by hand, including signs and zero.
minutes: 25
difficulty: hard
tags: strings, algorithms
id: c-data/text-and-numbers
mode: harness
---
`printf` turns numbers into text and `scanf` turns text into numbers, but how? You can do it yourself with two ideas.

## Digits are consecutive

In ASCII, `'0'` to `'9'` are consecutive codes, so `c - '0'` is the digit's value (`'7' - '0'` is 7). Going the other way, `'0' + 7` is `'7'`.

## Text to number

Read left to right. Each new digit pushes the earlier ones one decimal place up:

```c
int value = 0;
for (each digit d) {
    value = value * 10 + d;
}
```

"472" gives 4, then 47, then 472. A leading `-` or `+` sets the sign. Stop at the first character that is not a digit.

## Number to text

The last digit of a number is `n % 10`, and `n / 10` drops it. Repeating that yields the digits **backwards** (last digit first), so write them into the buffer in that order and then reverse the buffer, as in [Changing Arrays in Place](/learn/c-data/changing-arrays). Zero needs special care: the loop `while (n > 0)` would write nothing. Negative numbers: write a `'-'` first and work with the positive value.

Remember the terminator when you finish ([Copying and Comparing Strings](/learn/c-data/copy-compare)).

## Your turn

Don't use `atoi`, `strtol`, `sprintf` or `snprintf`. Inputs never overflow an `int`, and `int_to_text` is never given the smallest possible `int`.
