`find_index`: loop with `for (i, &x) in v.iter().enumerate()` and `return Some(i)` on a match; `None` after the loop. `first_even` can be `v.iter().copied().find(|x| x % 2 == 0)`.
===
`describe`: `match o { Some(n) => format!("got {}", n), None => "none".to_string() }`. `double_or_zero`: `o.map(|x| x * 2).unwrap_or(0)`.
===
`safe_div`: check `b == 0` first and return `Err("division by zero".to_string())`. `parse_age`: `s.parse::<u8>().map_err(|_| "not a valid age".to_string())`. Parsing straight into `u8` already rejects negatives and numbers above 255.
