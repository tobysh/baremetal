Keep `typedef struct {const char *type; size_t offset,length;} Carved;`. Implement `size_t carve(const uint8_t *p,size_t n,Carved *out,size_t max)`. Return the number stored, at most max, using the synthetic boundary rules above. Scan in offset order and skip complete recovered ranges. Empty data or max=0 returns 0.

Tests use several inputs, including boundary values. Keep the supplied function names and types.
