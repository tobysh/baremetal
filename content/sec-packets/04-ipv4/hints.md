Read the first byte once: `p[0] >> 4` is the version and `(p[0] & 15) * 4` is the header length in bytes. Check `n >= 20` before you touch `p[0]`.
===
Four comparisons protect you: `h >= 20`, `h <= n`, `total >= h`, `total <= n`. None of them is redundant, so test each one separately (IHL 4, IHL 15 in a 40-byte packet, total 19, total beyond `n`).
===
The fragment test is `(be16(p + 6) & 0x3fff) != 0`. Work out which bits 0x3fff keeps: 0x2000 is More Fragments, the low 13 bits are the offset, and 0x4000 (Don't Fragment) is excluded.
===
Build the result in a local struct and copy it to `*out` last. Cast to `uint32_t` before shifting address bytes so the first byte of 198.x or 203.x does not overflow an `int`.
