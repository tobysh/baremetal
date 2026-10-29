Keep UDP as supplied. Implement `int parse_udp(const uint8_t *p,size_t n,UDP *out)` returning 0 or -1 on NULL, truncation or an invalid declared length. Fill src/dst and payload=declared_length-8; ignore trailing bytes. Do not change out on failure.

Tests use several inputs, including boundary values. Keep the supplied function names and types.
