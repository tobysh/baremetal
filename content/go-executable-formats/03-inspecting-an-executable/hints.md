Mirror the object parser: do arithmetic on file values in `uint64` and report each rule with its own wrapped error.
===
Check order inside a segment matters only where two rules could both fire; the tests change one field at a time, so follow the order in the starter comment and you will pass.
===
Overlap of `[a, a+n)` and `[b, b+m)` is `a < b+m && b < a+n`. Compare against every earlier segment.
===
Entry: for each executable segment test `entry >= addr && entry-addr < memsz` (subtract after the first comparison so it cannot wrap).
===
`Describe`: `strings.Builder` and `fmt.Fprintf` with `0x%08x`. Build the permission string from the three flag bits.
