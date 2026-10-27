---
title: Formatting and Parsing Text
summary: Build text safely with snprintf, then read a tiny key=value language with a state machine that knows what it is looking at.
minutes: 40
difficulty: medium
tags: parsing, strings
id: c-practice/formatting-and-parsing
mode: harness
---
Programs constantly turn data into text (formatting) and text back into data (parsing). C has sharp edges for both, so this lesson gives you safe habits.

## snprintf: printf into a buffer, with a limit

`printf` writes to the screen. `snprintf` writes into a character array, and **never writes more than you allow**:

```c
char buf[16];
int n = snprintf(buf, sizeof(buf), "%02d:%02d", 7, 5);   // buf = "07:05"
```

- `%02d` means "an integer, at least 2 wide, padded with zeros". `7` becomes `07`.
- The second argument is the size of the buffer. At most `size - 1` characters are written, followed by the `'\0'`, so the result is **always a valid string** when the size is at least 1.
- The return value is how many characters the text *would* have needed, not counting the `'\0'`. If that number is `size` or more, the output was **truncated**:

```c
char tiny[4];
int n = snprintf(tiny, sizeof(tiny), "hello");   // tiny = "hel", n = 5
if (n < 0 || (size_t)n >= sizeof(tiny)) { /* it did not fit */ }
```

Never use plain `sprintf`, which has no limit and happily writes past the end of your array.

## Building a longer string piece by piece

Keep track of how much you have used. Each call writes at `buf + used` with `cap - used` space left:

```c
size_t used = 0;
for (...) {
    int w = snprintf(buf + used, cap - used, "%s;", word);
    if (w < 0 || (size_t)w >= cap - used) return -1;   // no room
    used += (size_t)w;
}
```

## Parsing with a state machine

Imagine reading `name=ada;age=36;`. As you scan character by character, what a character *means* depends on where you are: a letter before `=` belongs to the key, the same letter after `=` belongs to the value, and `;` ends a pair.

A **state machine** makes this explicit. You keep a variable saying what you are doing now, and each character either stays in the state or moves to another:

```
         '='                ';'
KEY  ------------>  VALUE ----------->  KEY   (a pair is finished)
 |  other char     |  other char
 +-> add to key    +-> add to value
```

```c
typedef enum { IN_KEY, IN_VALUE } State;
```

Then a `switch` on the state, inside the loop over characters, decides what each character does. Malformed input (a key with no `=`, an empty key, a stray `=` in a value) moves you into an **error** that stops the parse. Writing down all the "what if" rules before coding is half the work.

Copying into the fixed-size `key[16]` and `value[16]` arrays needs a length check on every character you add. A name of 16 characters would overflow, so reject it instead.

## Your turn

Write `format_time` (practise snprintf), `format_pairs` (the append loop) and `parse_pairs` (the state machine).
