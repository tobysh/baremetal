Think of the format as a table of 16-byte rows: row `i` starts at `16*i`, its name is `data[16*i : 16*i+12]` and its inode is `binary.BigEndian.Uint32(data[16*i+12:])`. Reject a bad total length before the loop.
===
To recover a name, find the first zero with `bytes.IndexByte(name, 0)`. A result of `-1` means all 12 bytes are the name. Then check that every byte after that index is zero, because trailing garbage would give one name two encodings.
===
Run each record through `validName` and the inode check, and keep a `map[string]bool` of names seen so far. Return `nil, ErrDir` the moment anything fails, so no partial slice escapes.
===
`EncodeDir` allocates `make([]byte, 16*len(entries))`, runs the same checks (validName, inode > 0, unique), then `copy(out[i*16:], e.Name)` and `PutUint32` at `i*16+12`. Because the output starts zeroed, the padding is already right.
