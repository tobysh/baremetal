Keep TCP as supplied. Implement `int parse_tcp(const uint8_t *p,size_t n,TCP *out)` returning 0 or -1 for NULL/truncated/invalid offset. Fill canonical src/dst ports, seq/ack, the raw byte-13 flags, header byte length and payload=n-header. Failure leaves out unchanged. The supplied span is a complete IP transport payload.

Tests use several inputs, including boundary values. Keep the supplied function names and types.
