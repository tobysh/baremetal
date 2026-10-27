Inside `mod counter` in the starter:

- `Counter::new(step: u32) -> Counter` starts at count `0`. A `step` of `0` would never move, so treat it as `1`.
- `tick(&mut self) -> u32` adds the step to the count and returns the new count. Use the private helper `bump` (given, with a signature to fill in) and make it saturate at `u32::MAX` instead of overflowing.
- `count(&self) -> u32` returns the current count.
- `reset(&mut self)` puts the count back to `0`, keeping the step.
- In the nested `pub mod format`, `label(c: &Counter) -> String` returns `"count=N"`, for example `"count=3"`. Reach the parent's `Counter` with `super::Counter`, and use its public `count()` method.

The fields stay private. Tests only use the public methods.
