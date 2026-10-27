For `Display`, `match self` and `write!(f, "...", ...)` in each arm. `NotANumber(e)` can write `{}` with `e`, because `ParseIntError` implements `Display` too.
===
`From` is a one-liner: `PortError::NotANumber(e)`. With it in place, `let n: i64 = trimmed.parse()?;` converts the parse error for you.
===
For `lookup`, `config.lines()` gives each line. `line.split_once('=')` returns `Option<(&str, &str)>`. A `for` loop with `if let Some((k, v)) = line.split_once('=')` is fine. Return `Some(v.trim())` for a matching key.
===
`port_from_config`: `let raw = lookup(config, "port").ok_or(PortError::Missing)?; parse_port(raw)`. And `halve_even`: `n.filter(|x| x % 2 == 0).map(|x| x / 2)`.
