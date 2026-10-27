In the starter, implement:

- `fn make_adder(n: i32) -> impl Fn(i32) -> i32` returns a closure that adds `n` to its argument.
- `fn apply_n<F: Fn(i32) -> i32>(f: F, x: i32, times: u32) -> i32` applies `f` to `x` the given number of times, feeding each result into the next call. With `times == 0` it returns `x` unchanged.
- `fn make_counter() -> impl FnMut() -> u32` returns a closure that gives `1` on its first call, `2` on its second, and so on. Each counter has its own independent count.
