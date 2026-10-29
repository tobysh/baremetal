Complete `parse_block`, `parse_if`, `parse_while`, `exec_block`, `exec_if` and `exec_while`.

Use braces for every branch/body and allow empty blocks. `else` is optional and takes a block. Missing braces must produce `expect`'s normal error. Conditions must be `Value::Bool`; otherwise return `TypeMismatch("condition must be a boolean".to_string())`.

Blocks execute in child environments. While conditions use the enclosing environment and are reevaluated before each iteration; each body executes in a fresh child. Propagate non-`Continue` flow immediately. Tests include zero iterations, nested scopes, outer-variable mutation, skipped branches and malformed blocks.
