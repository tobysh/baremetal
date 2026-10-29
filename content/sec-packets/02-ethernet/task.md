Keep Ethernet as supplied. Implement `int parse_ethernet(const uint8_t *p,size_t n,Ethernet *out)`, returning 0 on success and -1 on a NULL pointer or truncated header. Copy both MAC arrays, decode type, set payload offset 14 or 18 for one 0x8100 tag. Do not change *out on failure.

Tests use several inputs, including boundary values. Keep the supplied function names and types.
