`Display::fmt` is a `match self` with one `write!(f, ...)` per variant. Fields are bound by name in the pattern: `ConfigError::BadNumber { line, text } => write!(f, "line {line}: '{text}' is not a number")`.
===
In `parse_config` loop with `for (i, raw) in text.lines().enumerate()`, so the line number is `i + 1`. Skip with `continue` when the trimmed line is empty or starts with `#`.
===
Use `line.split_once('=').ok_or(ConfigError::MissingEquals(n))?`, then `val.parse::<i64>().map_err(|_| ConfigError::BadNumber { line: n, text: val.to_string() })?`. For duplicates check `out.iter().any(|(k, _)| k == key)`. After the loop, an empty `out` is `Err(ConfigError::NoSettings)`.
