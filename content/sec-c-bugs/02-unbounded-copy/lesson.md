---
title: Copying untrusted bytes
summary: Repair an unbounded copy by checking the destination capacity before changing memory.
minutes: 25
difficulty: easy
tags: security, defensive-programming
id: sec-c-bugs/unbounded-copy
mode: harness
---
An unbounded `memcpy(dst, src, n)` trusts the caller's length. A packet length or file field is a claim, not proof that the destination has that much room. An oversized copy can overwrite a neighbouring object even when the source contains no zero bytes.

Here the interface has two independent bounds: the source contains `n` readable bytes, and the destination has `cap` writable bytes. A byte copy differs from a string copy: there is no terminator and embedded zeros are ordinary data. Decide whether the entire request fits before copying anything. This exercise rejects an oversized request rather than silently truncating it.

Treat zero length as a successful no-op, including when either pointer is NULL. For a nonempty request, both pointers must be non-NULL. The buffers do not overlap; overlapping moves would need `memmove`. A failed operation must leave every destination byte unchanged, so validation belongs before the write. In review, look for every route from an external length to a memory operation and identify the actual destination capacity.
