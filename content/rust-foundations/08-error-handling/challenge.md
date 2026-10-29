**Challenge: report every problem, not just the first.**

`parse_sum` stops at the first bad item. A real tool would tell the user about all of them at once. Write

```rust
fn parse_report(input: &str) -> Result<i64, Vec<String>>
```

It takes the same comma-separated list. On success return `Ok(sum)`. Otherwise return `Err` with **one message per problem, in the order the items appear**. Items are numbered from 1.

- An empty or all-whitespace input is `Ok(0)`.
- An item (after trimming) that is empty gives `"item N is empty"`.
- An item that is not a valid `i64` gives `"item N is not a number: TEXT"`, where `TEXT` is the trimmed item. A number too large for `i64` counts as not valid.
- If every item is fine but the **sum** does not fit in an `i64`, return `Err` with the single message `"sum overflows"`. Use `checked_add`; a debug build panics on overflow, and a release build silently wraps.
- Do not stop at the first error, and do not report the overflow when there were item errors.

For example `"1, x, , 4"` gives `Err(vec!["item 2 is not a number: x", "item 3 is empty"])`.
