`parse_sum`: `Ok(a.trim().parse::<i32>()? + b.trim().parse::<i32>()?)`. For `sum_all` keep a running `total` in a `for` loop and use `?` inside it.
===
`first_even`: loop over the items, `let n: i32 = s.trim().parse()?;`, `if n % 2 == 0 { return Ok(Some(n)); }`, and after the loop return `Ok(None)`. Negative numbers: `-4 % 2 == 0`.
===
`parse_or`: `s.trim().parse().unwrap_or(default)`. `describe`: `match s.trim().parse::<i32>() { Ok(n) => format!("number {n}"), Err(e) => format!("error: {e}") }`.
