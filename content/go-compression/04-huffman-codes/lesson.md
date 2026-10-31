---
title: Huffman Code Lengths
summary: Build optimal Huffman code lengths with deterministic tie-breaking, then assign canonical codes from the lengths alone.
minutes: 45
difficulty: medium
tags: huffman, canonical-codes, determinism
id: go-compression/huffman-codes
mode: harness
---
**Huffman coding** gives frequent symbols short codes and rare ones long codes. Build a binary tree by repeatedly merging the two least frequent nodes; a symbol's code length is its depth.

## Why we keep only the lengths

Two things make a Huffman *code* portable:

1. **Canonical codes.** Given only the code *lengths*, assign codes in a fixed way: sort used symbols by `(length, symbol)`; the first gets all zeros; each next code is the previous plus one, shifted left whenever the length grows. A file needs to store only the lengths, not the tree. This is what DEFLATE does. RFC 1951 gives a worked example: lengths `3 3 3 3 3 2 4 4` for `A..H` give `A=010 B=011 C=100 D=101 E=110 F=00 G=1110 H=1111`. The hidden tests check it.
2. **Deterministic tie-breaking.** Frequencies tie constantly, and different tie-breaks give different (equally good) lengths. If an encoder is not deterministic, its output cannot be tested against a known answer or reproduced. Our rule: compare nodes by `(weight, smallest symbol inside the node)`, merge the two smallest. A lone symbol gets length 1, since a code needs at least one bit.

## Facts to check yourself

- A proper Huffman code is **complete**: `sum(2^-len) == 1` (Kraft sum). An **oversubscribed** set (`1 1 1`) cannot be a prefix code at all.
- The total cost `sum(freq*len)` is optimal. The tests compute it independently.

Limitation: an unbounded Huffman tree can be deeper than any fixed code length. We allow lengths up to `MaxCodeLen = 48`, enough for inputs under 2 GiB; real formats such as DEFLATE cap lengths at 15 with a more complex algorithm, which this track does not cover.