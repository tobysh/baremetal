Implement `int pcap_count(const uint8_t *p,size_t n)`. Return the record count or -1 for unsupported/malformed/truncated data, including trailing partial record headers. Apply the header and record rules above. Ignore timezone and sigfigs fields. Zero-record files are valid; NULL input is invalid. Counts in these fixtures fit int.

Tests use several inputs, including boundary values. Keep the supplied function names and types.
