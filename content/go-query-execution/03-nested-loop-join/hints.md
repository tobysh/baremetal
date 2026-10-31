Keep the current outer row in the struct (`nil` means "need a new one"). `Next` is a loop: if there is no current row, pull one (return false if the outer is exhausted) and `Open` the inner.
===
Inner loop: pull from the inner; if `pred(cur, r) == True` return `concatRows(cur, r)`. Resuming on the next `Next` call works because the inner iterator remembers its position.
===
When the inner is exhausted, `Close` it, set the current row to `nil` and go around again.
===
`Close` must also close an inner scan that is still open, for the case where the caller stops early.
