Implement `int token_equal(const char *hex,const uint8_t *expected,size_t n)`. Accept exactly 2*n hex digits (either case), no separators, and return 1 iff they decode to expected. Reject NULL hex, NULL expected with n>0, invalid digits or wrong length. Empty string matches n=0. Accumulate differences across all decoded bytes after syntax validation.

Tests use several inputs, including boundary values. Keep the supplied function names and types.
