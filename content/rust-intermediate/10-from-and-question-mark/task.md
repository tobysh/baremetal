Complete the `AppError` plumbing.

- `impl From<ParseIntError> for AppError` gives `BadInt(e)`; `impl From<ParseFloatError>` gives `BadFloat(e)`.
- `Display`: `bad integer: <inner message>`, `bad float: <inner message>`, `negative value: <n>`.
- `fn parse_scaled(count: &str, factor: &str) -> Result<f64, AppError>`: parse `count` as an `i64` and `factor` as an `f64` (both trimmed). A negative count is `Negative(count)`. The count is checked before the factor is parsed. Return `count * factor`. Use `?` for both parses.
- `fn total_scaled(pairs: &[(&str, &str)]) -> Result<f64, AppError>`: the sum of `parse_scaled` over the pairs, failing at the first error. An empty slice totals `0.0`.
- `fn collect_valid(pairs: &[(&str, &str)]) -> (Vec<f64>, Vec<String>)`: the successful values, and the `to_string()` of each error, both in input order.
