Implement `compile_expr` and `max_stack`.

Compile literals to `Const`/`Bool`, variables to `Load`, unary expressions to their operand code then `Unary`, and binary expressions to left code, right code, then `Binary`. Any call returns `CompileError::Unsupported("calls".to_string())`, including a call nested inside another expression.

`max_stack` receives valid straight-line expression bytecode: it never underflows. Return the highest depth reached from an empty initial stack, using the push/unary/binary effects in the lesson. Empty code has maximum zero.

Tests vary tree shapes, names, booleans, operand order, unsupported nested calls and stack depths.
