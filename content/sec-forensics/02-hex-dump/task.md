Implement `size_t hexdump(const uint8_t *p,size_t n,char *out,size_t cap)`. Rows are `%08zx: `, hex pairs separated by spaces in a 39-character padded field, two spaces, ASCII/dots, newline. Example one-byte row: `00000000: 41                                       A
`. Return the full required length, truncate safely, and terminate for cap>0. For cap=0 out may be NULL; p is valid for n bytes.

Tests use several inputs, including boundary values. Keep the supplied function names and types.
