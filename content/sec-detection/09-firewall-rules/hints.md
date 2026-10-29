Convert addresses to `uint32_t` and compare after masking both sides, so non-normalised networks work.
===
Treat `/0` specially: the mask is 0, not `0xFFFFFFFF << 32`.
