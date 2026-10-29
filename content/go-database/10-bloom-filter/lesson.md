---
title: A Bloom Filter
summary: Answer "is this key definitely not here?" in a few nanoseconds, with a bit array and a handful of hashes.
minutes: 35
difficulty: medium
tags: bloom-filter, hashing, bits
id: go-database/bloom-filter
mode: harness
---
The [LSM read-path lesson](/learn/go-database/lsm-reads) ended with a problem: looking up a key that does not exist costs one read per table. A **Bloom filter** fixes that. It is a small bit array that answers one question about a set of keys:

- **"definitely not in the set"**: always correct.
- **"maybe in the set"**: usually correct, sometimes a **false positive**.

It never gives a false negative. So before reading a table you ask its filter; "definitely not" lets you skip the disk entirely.

## How it works

Start with `m` bits, all zero, and `k` hash functions.

- **Add** a key: compute `k` bit positions and set those bits to 1.
- **MayContain** a key: compute the same `k` positions. If any bit is 0, the key was never added. If all are 1, it probably was (other keys may have set those bits).

You do not need `k` independent hash functions. Compute one 64-bit hash, split it into two 32-bit halves `h1` and `h2` (make `h2` odd so it is never zero), and use `h1 + i*h2` for `i = 0..k-1`. This is called double hashing. The standard library has what you need:

```go
h := fnv.New64a()
h.Write(key)
sum := h.Sum64()
h1, h2 := uint32(sum), uint32(sum>>32)|1
pos := (h1 + uint32(i)*h2) % m
```

## Sizing

You choose the budget as **bits per key**. With `n` keys you allocate `m = n * bitsPerKey` bits (rounded up to a multiple of 64 so they fit whole `uint64` words, with a minimum of 64). The best number of hashes is about `bitsPerKey * 0.69`, clamped to between 1 and 30. At 10 bits per key that gives `k = 7` and roughly a 1% false-positive rate.

## Storing it

A filter belongs next to its table, so it must serialise. The format here is: one byte `k`, four bytes `m` (big-endian), then the words as big-endian `uint64`s. Reading it back must validate the data, since it came from a file.
