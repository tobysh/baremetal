Write the three allocations first, each followed by `if err != nil`. For each of those branches, ask what has been allocated so far.
===
After `data` fails: nothing to free. After `index` fails: free `data`. After `scratch` fails: free `index` and `data`.
===
Right after `scratch` succeeds, `defer a.Free(scratch)`. That covers success and any later return.
===
Item `i` starts at the end offset of item `i-1` (0 for the first) and ends at its own offset. Encode with `binary.BigEndian.PutUint32(scratch, uint32(pos))` then `copy(index[4*i:], scratch)`.
