`src.lines().enumerate()` gives original line numbers even if you skip a blank line after enumerating. Match each `calc` result locally instead of using `?` for the whole batch.
===
Use `Option<i64>` for the total: `None` means no integer seen yet. For each integer, add with `checked_add` and propagate overflow as `None`.
