Keep IPv4 as supplied. Implement `int parse_ipv4(const uint8_t *p,size_t n,IPv4 *out)` returning 0 or -1, filling canonical numeric src/dst, total, protocol, header byte length, and fragmented as 0/1. Apply the version/IHL/total-length checks above; failure leaves out unchanged. Checksums and option syntax are outside this subset.

Tests use several inputs, including boundary values. Keep the supplied function names and types.
