Use `map[Value][]Row`. Interface values are comparable, and `int64(1)` and `"1"` are distinct keys, which is what the join needs.
===
Skip a build row if its key is `nil`; do the same on the probe side: a NULL probe key has no matches.
===
In `Next`: keep the current left row and its slice of matches plus an index. When the matches run out, pull the next left row and look it up.
===
Remember to reset the table, the built flag and the cursor in `Open`.
