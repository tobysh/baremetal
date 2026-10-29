Keep Meta as supplied. Implement `int read_meta(const uint8_t *p,size_t n,Meta *out)`: return 0 for the supported TIFF subset, -1 for a malformed header/directory/supported value; clear missing fields. Strings must fit (including terminator), numeric tags require count=1; unknown tags/types are ignored. Implement `long long to_unix(const char *s)` for valid Gregorian timestamps in years 1970..2100, interpreting UTC and returning -1 for invalid text/date.

Tests use several inputs, including boundary values. Keep the supplied function names and types.
