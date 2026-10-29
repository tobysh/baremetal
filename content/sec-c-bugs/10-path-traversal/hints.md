Copy segments into `out` as you go, keeping a write index. Popping a segment means moving that index back to the previous slash.
===
Before appending a name of length `l` (plus a slash when `w>1`), check `w + need + 1 <= cap`. Failure paths should set `out[0]=0`.
