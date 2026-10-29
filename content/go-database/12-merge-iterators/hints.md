`Live` and `Range` are small structs holding the inner iterator, with a `Next` method. `Live.Next` loops: `e, ok := it.Next()`; return if `!ok` or `!e.Delete`.
===
`Range`: keep a `done bool`. In `Next`, loop pulling entries; skip while `e.Key < start`; when `end != "" && e.Key >= end` set `done = true` and return false. After `done`, never call the inner iterator again.
===
`Merge` state: `heads []Entry` and `have []bool`, one slot per source, filled lazily. Write a `fill(i)` that pulls from source `i` if it has no head and is not exhausted. At the start of `Next`, fill every slot.
===
Each `Next` of `Merge`: scan the slots for the smallest key (use `<` so ties keep the lowest index, the newest). Remember that winner; then for every slot whose head has the same key set `have[i] = false`. If no slot has a head, the merge is finished.
===
The winning entry must be copied out before its slot is cleared, as the loop that clears the slots overwrites the state you read from.
