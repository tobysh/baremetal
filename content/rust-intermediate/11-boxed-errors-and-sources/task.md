Complete these.

- `fn read_pair(s: &str) -> Result<(i32, i32), Box<dyn Error>>`: parse `"a,b"` into two integers (each trimmed). If there is no comma the error's message is exactly `expected 'a,b'`; a bad number gives the parse error itself.
- `impl Error for LoadError`: report `self.source` from the `source()` method. `Display` is already written: `could not load '<name>'`.
- `fn load(name: &str, text: &str) -> Result<i32, LoadError>`: parse the trimmed text as an `i32`; on failure, wrap the parse error in `LoadError` with the given `name`.
- `fn load_all(entries: &[(&str, &str)]) -> Result<Vec<i32>, Box<dyn Error>>`: `load` each `(name, text)` pair and stop at the first failure.
- `fn chain(e: &dyn Error) -> Vec<String>`: the message of `e`, then the message of its source, then the source's source, and so on.
- `fn is_int_error(e: &(dyn Error + 'static)) -> bool`: true if the error is a `ParseIntError` (use `downcast_ref`).
