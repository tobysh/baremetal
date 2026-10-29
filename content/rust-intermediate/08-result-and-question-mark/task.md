All numbers are `i32`, and surrounding whitespace is ignored.

- `fn parse_sum(a: &str, b: &str) -> Result<i32, ParseIntError>`: parse both and add them; the first bad input is the error.
- `fn sum_all(items: &[&str]) -> Result<i32, ParseIntError>`: the sum of all items; `Err` at the first item that does not parse. An empty slice sums to 0.
- `fn first_even(items: &[&str]) -> Result<Option<i32>, ParseIntError>`: go through the items in order and return `Ok(Some(n))` for the first even number. If every item parses but none is even, `Ok(None)`. A bad item is an `Err` **only if it is reached** before an even number is found.
- `fn parse_or(s: &str, default: i32) -> i32`: the parsed number, or `default` if it does not parse.
- `fn describe(s: &str) -> String`: `"number 42"` for a valid number, or `"error: "` followed by the parse error's message.
