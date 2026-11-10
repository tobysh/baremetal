The 512-byte `disk` array is in the starter. Implement:

- `void disk_init(void)` sets every byte of the disk to 0.
- `int disk_read(size_t offset, void *dst, size_t len)` copies `len` bytes starting at `offset` from the disk into `dst` and returns 0.
- `int disk_write(size_t offset, const void *src, size_t len)` copies `len` bytes from `src` onto the disk starting at `offset` and returns 0.

A request is valid when `offset + len <= 512` in exact arithmetic. Both functions return -1 for an invalid request and copy nothing: the disk is unchanged after a failed write and `dst` is unchanged after a failed read. Your check must not overflow for huge values such as `offset = (size_t)-2` or `len = (size_t)-5`. A zero-length request at offset 512 is valid; any request at offset 513 or beyond is not. Buffers are valid for `len` bytes and do not overlap the disk.
