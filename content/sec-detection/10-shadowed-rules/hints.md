For each rule J, look at the earlier rules I = 1 to J-1 and ask whether I covers J completely. Stop at the first (lowest-numbered) one that does.
===
Covering means every field is at least as wide: protocol is `any` or equal, port range of I contains the range of J, and each network of I contains the network of J. Use the parse functions from the previous lesson to get a network and a mask for each side.
===
Network containment is `(maskA & maskB) == maskA` (A's mask is a subset of B's) and `(netB & maskA) == netA`. If a rule is reported as shadowed by a narrower rule above it, you tested containment in the wrong direction. If `deny any ...` is reported under a `tcp` rule, you treated `any` as equal to `tcp`.
===
Containment of two ranges on a different example, intervals `[a, b]` and `[c, d]`:
```c
int a_covers_c = a_lo <= c_lo && c_hi <= a_hi;
```
