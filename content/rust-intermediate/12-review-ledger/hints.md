`Display` is one `write!` per variant, as in the previous lessons. `source`: `match self { LedgerError::Amount { source, .. } => Some(source), _ => None }`.
===
In `apply`, for each non-blank line: `let (name, amt) = line.split_once(':').ok_or(LedgerError::Syntax(n))?;`, trim both, return `Syntax(n)` when the name is empty, then `amt.parse::<i64>().map_err(|e| LedgerError::Amount { line: n, source: e })?`.
===
`let bal = map.entry(name.to_string()).or_insert(0);` then `if *bal + amount < 0 { return Err(LedgerError::Overdraft { name: name.to_string(), balance: *bal + amount }); }` and `*bal += amount;`. `report`: on `Ok(m)` map each `(k, v)` to `format!("{k}={v}")` and `join(", ")`.
