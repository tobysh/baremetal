A callback has no closure state: everything it needs arrives through `ctx`. Decide first which type `ctx` really points to for each callback (`Stats`, `Search`), and how `for_each` stops.
===
`for_each`: handle the two null cases before the loop (note that `len == 0` comes first). Then read element `i` with pointer arithmetic (`xs.add(i)` counts elements, not bytes), call `cb`, and decide from its return value whether to continue. The count you return includes the call that asked to stop.
===
Inside a callback, turn `ctx` back into a typed reference with one cast, then update fields; `max` can be folded with `Option::map_or`. In `stats_of`, build a `Stats`, pass `&mut s` cast through `*mut c_void`, and return `s` afterwards. An empty slice has a non-null pointer, which is fine because `len` is 0.
===
If `first_negative` gives the wrong index, check which counter you bump: `seen` must increase only for non-negative values, so at the moment of the stop it already equals the index.
