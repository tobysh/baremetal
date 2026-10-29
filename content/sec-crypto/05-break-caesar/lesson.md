---
title: Breaking a Caesar cipher
summary: Read a shifted message from stdin, find the shift automatically, and print the plaintext. Then see how the same idea breaks Vigenere.
minutes: 30
difficulty: medium
tags: caesar, cryptanalysis, stdin
id: sec-crypto/break-caesar
mode: stdout
---
Julius Caesar reportedly shifted every letter of his messages by 3: A becomes D, B becomes E, and Z wraps around to C. That is the **Caesar cipher**, and it has 25 useful keys (a shift of 0 does nothing). Breaking it takes no cleverness: try them all.

```c
char shift_letter(char c, int s) {
    if (c >= 'a' && c <= 'z') return (char)('a' + (c - 'a' + s) % 26);
    if (c >= 'A' && c <= 'Z') return (char)('A' + (c - 'A' + s) % 26);
    return c;   // spaces, digits and punctuation stay as they are
}
```

To undo a shift of `s` you shift by `26 - s`. The `% 26` is the wrap-around.

## Finding the right one

As in the last lesson you need a score. Try each of the 25 shifts, decrypt, score the result, and keep the best. Here a simple score is enough: count the letters `e`, `t`, `a`, `o`, `i`, `n`, `s`, `h`, `r` in the result (any case). Right shift: lots of them. Wrong shift: few.

## Why Vigenere falls too

The **Vigenere cipher** uses a *keyword*: the first letter is shifted by the first key letter, the second by the second, and so on, repeating. For 300 years it was called "le chiffre indechiffrable". But if you know the key length `L`, every `L`th letter was shifted by the same amount, so those letters form a plain Caesar cipher. You split the text into `L` columns and break each column exactly as you do here. Guessing `L` is the only new step (look for the key length that makes repeated fragments line up, or whose columns have English-like letter statistics). This is the same attack as breaking repeating-key XOR.

The lesson, again: a cipher that keeps the statistics of the plaintext is broken. Modern constructions aim to make ciphertext computationally indistinguishable from random under their security assumptions. They still require correct modes, nonces and authentication. In production never roll your own crypto; use a vetted library.

Frequency scoring is a heuristic, not proof of the original message. Short text and unusual vocabulary can be ambiguous. These fixtures use sentences that the described scorer identifies; arbitrary ciphertext has no guaranteed unique English interpretation.
