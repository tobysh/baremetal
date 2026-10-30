Write `fn parse_sum(input: &str) -> Result<i64, String>`.

`input` is a list of integers separated by commas, such as `"1, 2,3"`. Return `Ok` with their sum.

- Trim whitespace around each item (`" 4 "` is 4). Negative numbers like `-5` are allowed.
- An empty input (or only whitespace) is `Ok(0)`.
- An empty item between commas, as in `"1,,2"`, is an error.
- If any item is not a valid integer, return `Err` with a message containing the offending item's text, e.g. `Err("invalid number: abc".to_string())`.
