Write six functions:

- `fn find_index(v: &[i32], target: i32) -> Option<usize>` is the position of the first element equal to `target`, or `None`.
- `fn first_even(v: &[i32]) -> Option<i32>` is the first even number, or `None`.
- `fn describe(o: Option<i32>) -> String` returns `"none"` for `None` and `"got N"` for `Some(N)`, e.g. `"got -3"`.
- `fn double_or_zero(o: Option<i32>) -> i32` doubles the number inside, or gives `0` for `None`.
- `fn safe_div(a: i32, b: i32) -> Result<i32, String>` is `Ok(a / b)`, or `Err` with the exact text `"division by zero"` when `b` is 0.
- `fn parse_age(s: &str) -> Result<u8, String>` parses a whole number from 0 to 255 (no surrounding spaces allowed, whatever `parse` accepts is fine). If parsing fails return `Err` with the exact text `"not a valid age"`, for any bad input including negatives and numbers above 255.
