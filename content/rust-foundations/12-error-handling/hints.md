Handle the empty case first: `if input.trim().is_empty() { return Ok(0); }`. Then split with `input.split(',')` and loop over the pieces, keeping a running total.
===
For each piece: `let item = piece.trim();` then `let n = item.parse::<i64>()`. The `Result` from `parse` has the wrong error type, so convert it with `.map_err(|_| format!("invalid number: {}", item))`.
===
Put the `?` after `map_err(...)` to return early on a bad item: `total += item.parse::<i64>().map_err(|_| format!("invalid number: {}", item))?;`. Finish with `Ok(total)`.
