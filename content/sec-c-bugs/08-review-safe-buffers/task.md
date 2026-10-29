Implement `int record_copy(uint8_t *dst,size_t cap,const uint8_t *src,int len)`. Return 1 and copy exactly len bytes plus one zero terminator. Return 0 for negative len, NULL dst, NULL src with positive len, or insufficient capacity. A failure writes nothing. Source and destination do not overlap.

Tests use several inputs, including boundary values. Keep the supplied function names and types.
