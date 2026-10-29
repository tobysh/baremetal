Keep Flow as supplied. Implement `size_t scan_sources(const Flow *f,size_t n,size_t threshold,uint32_t *out,size_t max)`. n<=128, valid spans. Count distinct ports per src only when `(flags & 0x12) == 0x02`. Store qualifying src values once, sorted numerically ascending, at most max; return count stored. Return 0 for threshold=0, n=0 or max=0. No network operations.

Tests use several inputs, including boundary values. Keep the supplied function names and types.
