Implement `void report(const char *name,const uint8_t *p,size_t n,uint64_t expected,char *out,size_t cap)`. Format exactly `name: TYPE, first bytes HEX, hash 16_lowercase_hex_digits, OK
` (or CHANGED). HEX is up to 8 lowercase byte pairs separated by spaces, or `-`. Types are PNG/JPEG/PDF/ZIP/unknown. Truncate safely and terminate if cap>0. Inputs and name are valid; out may be NULL only for cap=0.

Tests use several inputs, including boundary values. Keep the supplied function names and types.
