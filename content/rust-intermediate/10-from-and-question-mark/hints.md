Each `From` impl is one line: `AppError::BadInt(e)`. The `Display` match for `BadInt(e)` is `write!(f, "bad integer: {e}")`.
===
`parse_scaled`: `let n: i64 = count.trim().parse()?;` then `if n < 0 { return Err(AppError::Negative(n)); }` then `let k: f64 = factor.trim().parse()?;` and `Ok(n as f64 * k)`. The annotated type tells `parse` what to produce, and therefore which `From` the `?` uses.
===
`total_scaled` adds `parse_scaled(a, b)?` inside a loop. `collect_valid` can `match parse_scaled(a, b) { Ok(v) => values.push(v), Err(e) => errors.push(e.to_string()) }`.
