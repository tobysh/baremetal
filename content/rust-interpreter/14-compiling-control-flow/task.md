Implement `emit_stmt(stmt, code)`.

Compile `Let`, `Assign`, `Print` and `ExprStmt` as expression code followed by `Store`, `Assign`, `Print` or `Pop`. Emit block contents in order using the VM's flat global scope.

Use placeholder jumps and backpatch absolute indices for `If` and `While` exactly as explained in the lesson. Targets must remain correct when `code` already contains instructions and when branches nest. `Fn` and `Return` return `CompileError::Unsupported("functions".to_string())`. Propagate expression compiler errors unchanged.

The existing `compile_stmts` and `run_vm` compose your function with the parser and VM. Tests inspect jump targets, try nested bounded control flow, compare shared-subset programs with the interpreter, and verify unsupported constructs.
