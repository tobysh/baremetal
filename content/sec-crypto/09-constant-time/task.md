Implement `int equal_bytes(const uint8_t *a,const uint8_t *b,size_t n,size_t *visited)`. For valid inputs inspect all n byte pairs, return 1 iff equal, and set *visited to n if supplied. Zero length is equal, including NULL pointers. If n>0 and either pointer is NULL, return 0 and set visited to 0. Use XOR and OR accumulation with no early return on a differing byte.

Tests use several inputs, including boundary values. Keep the supplied function names and types.
