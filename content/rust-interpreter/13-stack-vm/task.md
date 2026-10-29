Implement `Vm::run` for every supplied `Op` variant.

Use `self.pop()` for consuming operands, preserving its error. Binary operators pop right then left and call `apply_binary`; unary operators call `apply_unary`. `Load` clones an existing global or returns `Undefined`. `Store` defines/replaces; `Assign` requires an existing name. Both consume one value. `Print` and `Pop` also consume one value.

Jump targets are absolute indices. `JumpIfFalse` consumes a boolean and jumps on false; other kinds return `TypeMismatch("condition must be a boolean".to_string())`. `run` preserves stack, globals and output across calls and stops on the first error. Tests include operand order, checked overflow, stack underflow, globals, jumps and repeated runs.
