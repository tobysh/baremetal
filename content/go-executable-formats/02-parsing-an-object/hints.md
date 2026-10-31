Make a small helper `name(off uint32) (string, error)` that checks `off < len(str)`, then `bytes.IndexByte(str[off:], 0)`. Slice the string table once: `str := b[h.StrOff:h.End]`.
===
Entry `i` of a table starts at `b[h.SecOff+uint64(i)*16:]`. The header check already proved the whole table is inside `b`, so slicing is safe there.
===
For section contents compare in `uint64`: `uint64(off)+uint64(size) > uint64(len(b))`. Then `append([]byte{}, b[off:off+size]...)` copies.
===
Parse sections before symbols and symbols before relocations: each later table checks indexes against the earlier ones.
===
Edge cases the tests use: a symbol whose value equals the section size; a relocation ending exactly at the section end; `0xFFFFFFF0` as an offset; a global symbol with name offset 0 (the empty string).
