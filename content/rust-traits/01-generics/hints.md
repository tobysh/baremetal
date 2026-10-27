For `largest`, start with `let mut best = *items.first()?;` or match on `items.first()` so an empty slice returns `None`. Because `T: Copy`, `*item` copies a value out of a reference.
===
Loop over `items` and compare each with `best`: `if *item > best { best = *item; }`. Finally return `Some(best)`.
===
`swap` takes `self` by value, so you can move both fields: `Pair { first: self.second, second: self.first }`.
