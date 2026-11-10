Implement `size_t heap_map(char *out, size_t cap)` in the supplied heap. Everything above it in the starter is given; do not edit it.

**Map format.** Walk the blocks in address order. Each block emits one `H`, then `size / 16` characters: `.` if the block is free, `U` if it is in use. There are no separators or newlines. A fresh 256-byte heap gives `H...............` (a 240-byte payload is 15 units).

**Return value.** The number of characters the complete map needs, excluding the terminating NUL. It must be the same for every `cap`, including 0, and must never depend on how much was written.

**Writing.**
- `cap == 0`: write nothing. `out` may be `NULL`.
- `cap > 0`: write the first `min(needed, cap - 1)` map characters, then a NUL right after them. Never write at or beyond `out[cap]`.

**Edge cases.** A heap of size 0, or smaller than one header plus one payload unit (32 bytes), has no blocks: return 0 and, if `cap > 0`, write an empty string. A buffer size that is not a multiple of 16 is rounded down by `heap_init`, and the map reflects that.

**Read-only.** Do not change block headers, `steps`, `failed`, or any heap byte. Calling `heap_map` any number of times must leave `heap_stats` and the payload bytes identical.

The hidden tests build heaps of several sizes with allocations, frees and coalescing, compare the map to the expected string, check every buffer size around the boundary for overruns, and confirm the map agrees with `heap_stats`.
