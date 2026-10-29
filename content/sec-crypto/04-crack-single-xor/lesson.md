---
title: Cracking single-byte XOR
summary: Recover a secret key by trying all 256 and scoring which result looks like English.
minutes: 30
difficulty: medium
tags: xor, cryptanalysis, frequency
id: sec-crypto/crack-single-xor
mode: harness
---
Suppose someone XORs a whole message with **one** secret byte. There are only 256 possible keys, so an attacker simply tries them all. The hard part is knowing which result is the real message. A computer cannot read, but it can **score** text by how English-like it is.

## Scoring English

Natural language has a very uneven letter distribution. In English the space is the most common character, then `e`, `t`, `a`, `o`, `i`, `n`. Random garbage has no such pattern, and wrong keys also tend to produce control characters that never appear in normal text.

A simple scorer:

- give a letter points based on how common it is (`e` high, `z` low, ignore case);
- give a space a lot of points;
- give small points for common punctuation;
- subtract a big penalty for bytes that are not printable (below 32 except `\n`, or above 126).

Then try every key `k` from 0 to 255, compute `plain[i] = cipher[i] ^ k`, score it, and keep the best.

```c
int best_key = 0;
long best_score = -1000000;
for (int k = 0; k < 256; k++) {
    long s = score(cipher, n, (uint8_t)k);
    if (s > best_score) { best_score = s; best_key = k; }
}
```

This is **frequency analysis**, the oldest trick in cryptanalysis. It works because the cipher preserves the *shape* of the plaintext: a repeated key byte maps each plaintext letter to one fixed ciphertext byte, so the statistics survive. A good cipher destroys that shape completely.

## Lessons

A secret key space of 2^8 is nothing. Real keys are 128 or 256 bits: 2^128 guesses are out of reach for anyone. But note the second lesson: even when the key space is large, a cipher that leaks statistics (like a short repeating key) can be broken without trying every key. Next you will do the same for Caesar shifts.

The English score is a heuristic: very short or non-English inputs can have several plausible keys. These fixtures use longer English sentences. A successful demonstration here does not make frequency analysis a universal decoder.
