The `Pipe` type (16 data bytes, `head`, `count` and a `closed` flag for the writing end) is in the starter. Implement:

- `void pipe_init(Pipe *p)` makes the pipe empty with its writer open, whatever its fields held before.
- `int pipe_write(Pipe *p, const void *src, size_t n)` appends as many of the `n` bytes as fit (the pipe holds at most 16) and returns how many it appended, which may be fewer than `n` or 0 when the pipe is full. After `pipe_close_writer` it returns -1, even for `n == 0`.
- `int pipe_read(Pipe *p, void *dst, size_t n)` removes up to `n` of the oldest bytes into `dst`, in the order they were written, and returns how many it removed. A read of `n == 0` returns 0. When the pipe is empty it returns -1 while the writer is open (try again) and 0 once the writer has closed (end of file). Bytes written before the close are still read before EOF.
- `void pipe_close_writer(Pipe *p)` closes the writing end.

Buffers are valid for `n` bytes and do not overlap the pipe.
