Start with a helper that builds the unshifted mask for a width: `width >= 32 ? 0xFFFFFFFFu : (1u << width) - 1u`.
===
`field_get` is `(read >> shift) & mask`. Read the register once and keep the value in a local variable.
===
`field_set`: read once into `old`, compute `field = mask << shift`, then write `(old & ~field) | ((val & mask) << shift)`. Mask `val` before shifting.
