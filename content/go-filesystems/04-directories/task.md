Implement `EncodeDir(entries []DirEntry) ([]byte, error)` and `DecodeDir(data []byte) ([]DirEntry, error)` with the 16-byte record format from the lesson. The starter provides `validName` (1-12 bytes, none `NUL`, `/` or above 127) and `ErrDir`.

- `EncodeDir` emits one record per entry, **in the given order**, name zero-padded to 12 bytes, inode big-endian. It returns `ErrDir` and a `nil` slice if any name is invalid, any inode number is 0, or two entries have the same name (case sensitive). An empty or `nil` input is valid and encodes to zero bytes.
- `DecodeDir` returns the entries in on-disk order, or `ErrDir` and a `nil` slice if: `len(data)` is not a multiple of 16; a name is empty (first byte 0); any byte after the first `NUL` of a name is not zero; a name contains `/` or a byte above 127; an inode number is 0; or a name repeats. A name that fills all 12 bytes has no `NUL` and is valid. Empty input is a valid empty directory.
- Neither function modifies its argument, and `DecodeDir` must not return slices that alias `data`.

`Decode(Encode(x))` equals `x` for every valid `x`. The tests check exact bytes, many sizes, each rule on its own, and that inputs are left unchanged.
