After consuming `{`, loop until `peek()` is `RBrace` or EOF, then call `expect(RBrace)` so EOF uses the standard diagnostic.
===
`exec_block` can call `exec_stmts(stmts, &env.child())`. For a loop, match the condition each time, then match the returned flow before repeating.
