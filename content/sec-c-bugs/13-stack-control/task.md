Implement `size_t make_payload(uint8_t *out, size_t cap, size_t offset, uint64_t win)` and `int safe_copy(uint8_t *frame, size_t offset, const uint8_t *input, size_t n)`.

For make_payload, return offset+8 and write that many bytes: any padding followed by win in eight little-endian bytes. If out is NULL, offset+8 overflows size_t, or cap is insufficient, return zero without writing. No NUL terminator is needed.

For safe_copy, reject n>offset without modifying frame. For n==0 return one (even with NULL pointers). Otherwise reject NULL pointers; copy exactly n bytes and return one. Caller supplies a valid frame of at least offset+8 bytes and nonoverlapping input when n>0. Hidden tests copy your payload into the model, decode the saved return slot and assert that the fictional win branch is selected. They also check the repaired copy preserves the return slot and guard bytes.
