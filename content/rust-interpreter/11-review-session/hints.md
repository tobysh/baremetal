Use `Result<Option<Value>, Error>` to represent the two successful paths: `Some` for an expression to echo, `None` for a program.
===
Drain output before matching the result. This preserves prints before a runtime error and clears them for the next call. A new transcript needs one session outside its loop.
