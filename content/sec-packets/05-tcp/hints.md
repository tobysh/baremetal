The header length is in the high nibble of byte 12, counted in four-byte words: `(p[12] >> 4) * 4`. Reading `p[12]` and multiplying without the shift is the most common mistake.
===
Guard in two steps: `n >= 20` before reading anything, then `header >= 20 && header <= n` before using the header length for the payload. Only then is `n - header` safe.
===
Write `be16` and `be32` helpers, with each byte cast to `uint32_t` before shifting. `seq` and `ack` use all 32 bits, so a sequence number such as 0xffffffff must come out unchanged.
===
`flags` is simply `p[13]`. Store it raw; do not mask it, because the test sends bytes with the ECE and CWR bits set.
