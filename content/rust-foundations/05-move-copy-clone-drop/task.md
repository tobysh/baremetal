The starter defines `Guard { name, log }` and `Guard::new`. Complete:

- `impl Drop for Guard`: when a guard is dropped, push the string `"drop NAME"` (for example `"drop a"`) onto its log.
- `fn clone_all(items: &[String]) -> Vec<String>` returns independent copies of every item, in order.
- `fn with_added(v: Vec<String>, extra: String) -> Vec<String>` takes ownership of both, appends `extra` to the end and returns the vector. No cloning is needed.
- `fn run_scope(log: &Log, first: &str, second: &str)` does this, in order: create a guard named `first`, create a guard named `second`, call `drop` on the first guard explicitly, push `"work"` onto the log, then return (which drops the second guard automatically).

So `run_scope(&log, "a", "b")` leaves the log as `["drop a", "work", "drop b"]`. The tests try several names.
