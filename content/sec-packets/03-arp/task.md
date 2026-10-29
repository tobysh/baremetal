Keep ARP as supplied. Implement `int parse_arp(const uint8_t *p,size_t n,ARP *out)`: return 0 for the complete supported request/reply subset, -1 for NULL, truncation or other header values. Set opcode, sender and target. Do not change *out on failure; extra trailing bytes are allowed.

Tests use several inputs, including boundary values. Keep the supplied function names and types.
