In `new`, pick the step with `if step == 0 { 1 } else { step }` (or `step.max(1)`), then build `Counter { count: 0, step }`.
===
`bump(count, step)` is a private function in the same module, so `tick` can call it directly. `count.saturating_add(step)` does the saturating arithmetic for you.
===
In `mod format`, `use super::Counter;` brings the parent's type into scope. Then `format!("count={}", c.count())` builds the string.
