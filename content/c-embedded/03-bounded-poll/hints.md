Record `uint32_t start = b->now(b);` before the loop and read the register inside it, not before.
===
The test for giving up is `(uint32_t)(b->now(b) - start) > timeout`. Do it only after a read that did not satisfy the mask.
===
Do not compare `now` with a precomputed `start + timeout`: that breaks when the sum wraps past 2^32.
