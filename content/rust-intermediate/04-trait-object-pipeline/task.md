Four stages share the `Stage` trait. Implement them:

| Stage | `name()` | `apply(x)` |
| --- | --- | --- |
| `AddN(n)` | `add:<n>` | `Some(x + n)`, or `None` on overflow |
| `MulN(n)` | `mul:<n>` | `Some(x * n)`, or `None` on overflow |
| `NonNeg` | `nonneg` | `Some(x)` if `x >= 0`, otherwise `None` |
| `Clamp(m)` | `clamp:<m>` | `Some(x)` capped to at most `m` |

Then write the helpers.

- `fn build(spec: &str) -> Option<Box<dyn Stage>>`: parse `add:5`, `mul:3`, `nonneg` or `clamp:30` (the argument may be negative). Anything else, a missing or non-numeric argument, or an argument on `nonneg` gives `None`.
- `fn build_all(specs: &[&str]) -> Option<Vec<Box<dyn Stage>>>`: all of them, or `None` if any one is invalid.
- `fn run(stages: &[Box<dyn Stage>], x: i64) -> Option<i64>`: pass `x` through the stages in order; `None` as soon as one stage says `None`. No stages returns `Some(x)`.
- `fn names(stages: &[Box<dyn Stage>]) -> Vec<String>`: the stage names in order.
