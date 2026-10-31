Work in `uint64` while laying out and check the limit before converting to `uint32`.
===
Write one helper that places all sections of one kind and returns the cursor. Text starts at cursor 0 relative to `Base`; data starts at 0 relative to `DataAddr`; bss continues from the data cursor.
===
`alignUp(v, 4)` is `(v+3)/4*4`. `DataAddr` aligns the *address* (`Base+TextSize`), not the size.
===
In `Relocate`, first copy all sections, then loop over relocations. To find a field's bytes: the segment slice for the section starts at `plan.Addr(obj,sec) - segmentBase`, then add `r.Off`.
===
Use `int64` for S, A and P so negative results are visible, then range-check before `uint32(v)` or `uint32(int32(v))`. Remember `P+4` in the relative formula.
