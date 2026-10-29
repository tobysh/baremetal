---
title: Predicting a weak random number generator
summary: See three outputs of a simple generator, work out its hidden state, and predict the next value before it happens.
minutes: 35
difficulty: hard
tags: prng, random, lcg, attack
id: sec-crypto/predict-lcg
mode: harness
---
Programs often need random numbers: session tokens, password reset codes, encryption keys. The classic `rand()` is a **pseudo-random** generator. It holds a number called the *state* and advances it deterministically. Determinism also appears in secure generators; the difference is whether observations make the hidden state or future output practical to recover. Anyone who knows the state knows every future output.

The most common formula is the **linear congruential generator** (LCG):

```c
state = state * 1664525 + 1013904223;   // all uint32_t, wraps modulo 2^32
```

Many libraries then hand out only some bits of the state, because the low bits of an LCG are especially weak (the lowest bit just alternates 0, 1, 0, 1). Here the output is the **top 16 bits**:

```c
uint16_t output = state >> 16;
```

It feels safe: 16 of 32 bits are hidden. It is not. After seeing one output you know the top half of the state and only the bottom 16 bits are missing: 65,536 possibilities. A computer tries all of them in a blink. Which one is right? Use the next outputs: for each candidate state, step it forward and check that its top half matches the second and third outputs you saw. Wrong candidates fail quickly; usually one survives.

```c
for (uint32_t low = 0; low < 65536; low++) {
    uint32_t s = ((uint32_t)o1 << 16) | low;   // state right after o1
    // advance s twice, compare with o2 and o3 ...
}
```

Once you know the state, you simply keep stepping it forward and **predict** the fourth output.

## Why this matters

If a website makes password reset tokens or session ids with a predictable generator, an attacker who collects a few of their own tokens can compute everyone else's. Other noncryptographic generators require different recovery methods; this exercise establishes the failure of the particular LCG above.

Security-sensitive random numbers must come from the operating system's **cryptographically secure** generator: `getrandom()` or `/dev/urandom` on Linux, or a library call such as libsodium's `randombytes_buf`. Never roll your own.
