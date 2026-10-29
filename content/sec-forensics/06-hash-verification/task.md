Implement `uint64_t fnv1a64(const uint8_t *p,size_t n)` and `int audit(const char *manifest,const Item *items,int n,int *status)`. Set one status per item (0 match, 1 changed, 2 unlisted) and return how many statuses are nonzero. Manifest lines are valid, unique, at most 255 characters, and filenames have no spaces. n is nonnegative. Keep the Item type supplied.

Tests use several inputs, including boundary values. Keep the supplied function names and types.
