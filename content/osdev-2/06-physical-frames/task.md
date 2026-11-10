The pool has 64 frames of 4096 bytes starting at `BASE` (`0x100000`), tracked by the bitmap `used` (bit `i` set means frame `i` is unavailable). Implement:

- `void frame_init(void)` marks every frame free.
- `void frame_reserve(unsigned first, unsigned count)` marks every frame in the index range `[first, first + count)` that lies inside the pool. Parts of the range past frame 63 are ignored, a range entirely outside the pool does nothing, and `count == 0` does nothing. Reserving an already unavailable frame is harmless. `first + count` may not fit in an `unsigned`: `frame_reserve(10, 0xFFFFFFFF)` must reserve frames 10 to 63.
- `uint64_t frame_take(void)` finds the lowest free frame, marks it unavailable and returns its address. It returns 0 and changes nothing when every frame is unavailable.
- `int frame_release(uint64_t addr)` returns 0 and frees the frame when `addr` is the exact start of an unavailable frame in the pool (taken or reserved). It returns -1 and changes nothing for an address below `BASE`, at or above `BASE + 64 * 4096`, not 4096-aligned relative to `BASE`, or for a frame that is already free.

The tests reserve overlapping and out-of-range spans, exhaust the pool, and release valid and invalid addresses.
