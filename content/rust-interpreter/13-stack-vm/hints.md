Increment `ip` before matching the instruction, then overwrite it only for a taken jump. Use `while ip < code.len()`.
===
Pop into local variables before pushing results to avoid overlapping mutable borrows. An assignment must check the map without silently inserting an absent name.
