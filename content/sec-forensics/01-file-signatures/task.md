Implement `const char *identify(const uint8_t *p, size_t n)`.

`p` points to `n` readable bytes (`n` may be zero). Return exactly one of these string literals: `"PNG"`, `"JPEG"`, `"GIF"`, `"PDF"`, `"ZIP"`, `"ELF"` or `"unknown"`.

- Use the signatures from the lesson table. Both GIF variants return `"GIF"`.
- Compare at offset zero only, using all signature bytes, and only when `n` is at least the signature length. A signature that is cut off by `n` does not match, even if the bytes in memory beyond `n` would complete it.
- Anything else, including the empty-archive ZIP marker `50 4b 05 06` and case variants such as `%pdf-`, is `"unknown"`.
- Never return NULL and never read at or beyond `p + n`.

The hidden tests try every signature at its exact length, one byte short, shifted by one byte, with near misses and with arbitrary bytes after the signature. Keep the supplied function name and types.
