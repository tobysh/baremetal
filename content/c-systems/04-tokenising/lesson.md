---
title: Tokenising Text by Hand
summary: Split a sentence into words and turn digit strings into numbers, walking the characters yourself with no strtok and no atoi.
minutes: 25
difficulty: medium
id: c-systems/tokenising
mode: harness
---
Programs are constantly handed text that needs taking apart: a command typed by a user, a line of a config file, a message from the network. The first step is **tokenising**, chopping the text into meaningful pieces called **tokens**.

C has `strtok`, but it quietly modifies your string and remembers state between calls, which causes confusing bugs. Writing the scanner yourself is easy, and it teaches you the pattern that every parser is built from.

## The cursor pattern

Keep an index `i` that walks along the string like a finger under a line of text. At each step, ask what is under the finger:

```c
int i = 0;
while (s[i] != '\0') {
    if (s[i] == ' ') {
        i++;                 // a gap: skip it
    } else {
        int start = i;
        while (s[i] != '\0' && s[i] != ' ') {
            i++;             // inside a word: move to its end
        }
        // the word is s[start] up to (not including) s[i]
        int length = i - start;
    }
}
```

Two nested loops: the outer one skips gaps and finds where a word starts, the inner one finds where it ends. The length is just the difference of the two positions.

## Copying a token out

Because you are not allowed to change the original, copy each token into its own buffer, and do not forget the terminator:

```c
for (int k = 0; k < length; k++) {
    word[k] = s[start + k];
}
word[length] = '\0';
```

When writing into a fixed-size buffer, always check `length` against the space available, otherwise a long word will spill over into the neighbouring memory.

## From digits to a number

A character like `'7'` is really the number 55 (its ASCII code). Because the digits `'0'` to `'9'` are stored in order, `'7' - '0'` is the number 7. To build a whole number, process one digit at a time, shifting what you have so far left by a decimal place:

```c
int value = 0;
for each digit d:
    value = value * 10 + (d - '0');
```

For `"472"`: 0, then 4, then 47, then 472. Validation matters too: `"12a"` and `""` are not numbers, and a program that accepts them will misbehave later. Check that every character is a digit, and report failure to the caller rather than guessing.

## Your turn

Write the two halves of a tiny command parser: `split_words` and `parse_int`.
