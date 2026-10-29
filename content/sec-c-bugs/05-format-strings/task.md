Implement `size_t literal_message(char *dst, size_t cap, const char *text)`. Store text literally, truncating to `cap-1` bytes and terminating when `cap > 0`. Return the full text length. NULL text means empty; for `cap == 0`, write nothing and allow NULL dst. For positive cap the caller supplies a valid destination.

Tests use several inputs, including boundary values. Keep the supplied function names and types.
