**Challenge: safe chunks.**

Write `Chunk(s []int, n int) [][]int`. It splits `s` into consecutive pieces of `n` elements; the last piece may be shorter.

- every chunk must **share elements** with `s` (writing to a chunk changes `s`), so do not copy;
- but appending to a chunk must **never overwrite** the next chunk or any other element of `s`, so each chunk's capacity must equal its length;
- if `n <= 0` or `s` is empty, return an empty, non-nil `[][]int`.

The full slice expression `s[low:high:max]` from the lesson does the work, once per chunk. Watch the last chunk: `high` must not run past `len(s)`.
