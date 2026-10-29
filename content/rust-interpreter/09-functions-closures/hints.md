Create the function with `env: env.clone()` and define it in the same frame. Capturing a clone of the map would break recursion and mutable closures.
===
`call_function` handles arity and converts `Flow::Return(v)` to `v`. `eval_call` handles expression evaluation order, collecting argument values before dispatching.
