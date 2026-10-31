Start with design A: a lower-bound binary search that counts one probe per step gives you `sa_has`, `sa_insert` (then shift) and both ends of `sa_range`.
===
For B, `i = hash_key(k)`; loop: count a probe, return on an empty slot (insert there) or on a match, otherwise `i = (i + 1) % SLOTS`. `hb_range` reads each slot once: add `SLOTS` to `probes`.
===
`choose_design`: write a helper that applies one op to a design; run all ops against both, sum `probes + moves + init`, check the memory limit, and compare. Use static sets so you do not blow the stack.
