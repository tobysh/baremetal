Complete the starter:

- `add`, `mul` and `max`: `extern "C"` functions of two `i32`. `add` and `mul` use wrapping arithmetic. `max` returns the larger value.
- `lookup(name: &str) -> Option<BinOp>`: `"add"`, `"mul"` and `"max"` give the matching function, anything else gives `None`. Names are case-sensitive.
- `fold_with(op: BinOp, init: i32, xs: &[i32]) -> i32`: start from `init` and replace the accumulator with `op(acc, x)` for each `x` in order. An empty slice returns `init`.
- `names() -> Vec<&'static str>`: the three names in alphabetical order, taken from one table so `lookup` and `names` cannot disagree.

Other operations and inputs are tried, including a function the tests define.
