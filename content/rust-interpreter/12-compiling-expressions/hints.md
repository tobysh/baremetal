Each recursive result is a `Vec<Op>`. Extend the left vector with the right vector, then push the operator.
===
Maintain `depth` and `deepest`. Unary changes neither; a binary instruction reduces depth by one after replacing its two operands with one.
