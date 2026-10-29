---
title: Off by One
summary: A loop that runs one step too far writes a byte past the end of a buffer. Learn why a single byte is enough to be dangerous, and fix a copy loop.
minutes: 20
difficulty: easy
tags: buffer-overflow, off-by-one
id: sec-c-bugs/off-by-one
mode: harness
---
Welcome to the security path. In this course you play both sides: first you read code and spot the bug, then you fix it. Everything here is toy code in a sandbox, and the point is always the same, to learn what goes wrong so you never write it.

C does not check array bounds. If you write to `buf[8]` when `buf` has 8 elements, nothing stops you: the byte lands on whatever happens to live next to the array. That is called a **buffer overflow**, and it is the oldest family of security bugs there is.

## The classic: one step too far

```c
void copy_label(char *dst, size_t cap, const char *src) {
    size_t i;
    for (i = 0; i <= cap && src[i] != '\0'; i++) {
        dst[i] = src[i];
    }
    dst[i] = '\0';
}
```

`dst` has room for `cap` bytes, numbered `0` to `cap - 1`. The loop condition `i <= cap` lets `i` reach `cap`, which is one past the end. Then the terminating `'\0'` can be written even further out. The two classic mistakes:

- **`<=` instead of `<`** in a loop bound.
- **Forgetting the terminator.** A string of 8 characters needs 9 bytes, so a buffer of `cap` bytes holds at most `cap - 1` characters.

## Why one byte matters

One stray byte sounds harmless, but memory is crowded. In concept, an attacker who controls the input might overwrite:

- the next variable (a length, a flag such as `is_admin`, a pointer),
- the low byte of a saved pointer or a size field in malloc's bookkeeping,
- the terminator of the *neighbouring* string, so that string suddenly runs on into other data.

Real programs have been taken over through exactly this kind of single-byte write. The compiler flag `-fstack-protector-strong` (which this sandbox uses) places a secret "canary" next to some stack buffers precisely to catch overflows, but it is a safety net, not a licence to be sloppy.

## The fix

Decide the rule first: **write at most `cap - 1` characters and then one `'\0'`**, so the result is always a valid, terminated, in-bounds string. Handle the edge cases explicitly:

- `cap == 0` means there is no room even for the terminator, so write nothing at all.
- `src == NULL` is treated as an empty string.

Then the loop bound becomes `i < cap - 1`, which is safe because you already returned when `cap` was 0 (so the subtraction cannot wrap around).

## How the tests catch it

The hidden tests put your buffer inside a larger block whose surrounding bytes are filled with a known pattern, call your function with hostile inputs (empty, exactly one short of full, exactly full, much too long, `cap` of 0 and 1), then check that no guard byte changed. Nothing needs to crash for the overflow to be detected.
