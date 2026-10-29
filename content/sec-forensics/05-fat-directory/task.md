Keep `DirEntry` as supplied. Implement `int read_dir(const uint8_t *p,size_t n,DirEntry *out,size_t max)`. Apply the record rules above, retain deleted entries with _ as first character, and return the number stored (at most max). Ignore incomplete trailing bytes; stop at first 0x00 record. For this subset names are ASCII, and first-byte 0x05 escaping is not used.

Tests use several inputs, including boundary values. Keep the supplied function names and types.
