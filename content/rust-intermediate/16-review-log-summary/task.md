Finish the parallel log summary.

- `LogError::BadLine(n)` displays as `line <n>: unknown level`.
- `fn level_of(line: &str, number: usize) -> Result<&str, LogError>`: the first whitespace-separated word if it is exactly `INFO`, `WARN` or `ERROR`; otherwise `Err(LogError::BadLine(number))`. Leading whitespace is ignored. The returned `&str` is a slice of `line`.
- `fn summarize(lines: Vec<String>, workers: usize) -> Result<BTreeMap<String, usize>, LogError>`: count how many lines have each level, using `workers` threads (treat 0 as 1). The first line is line 1. If any line is bad, return `BadLine` with the lowest bad line number. No lines gives an empty map. The result must be the same for every number of workers.
