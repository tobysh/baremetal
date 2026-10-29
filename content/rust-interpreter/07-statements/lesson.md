---
title: "Statements and Program Execution"
summary: Parse declarations, assignments and prints, then execute a sequence against one environment.
minutes: 45
difficulty: medium
tags: statements, parser, interpreter
id: rust-interpreter/statements
mode: harness
---
Builds on [Environments and Lexical Scope](/learn/rust-interpreter/environments).

Expressions produce values; statements organize a program. A declaration stores a value, an assignment changes a binding, a print adds output, and an expression statement evaluates a value then discards it. Every one ends with a semicolon in this language.

`Stmt` represents these shapes. `parse_stmt` inspects the next token: `Let` begins a declaration, `Print` begins a print, and an identifier followed by `Eq` begins an assignment. All other expression starts become expression statements. The lookahead distinguishes `x = 3;` from `x == 3;` without consuming the name too early.

A declaration has grammar `let NAME = EXPR;`; assignment omits `let`. Reuse `expect_ident`, `expect` and `parse_expr` rather than reproducing cursor logic. Parsing builds a whole program before execution, so a syntax error later in the source prevents any statements from running.

`exec_stmt` evaluates expression operands in the current environment. A declaration calls `define`; an assignment calls `assign` and returns `Undefined` if there is no target binding. Evaluate the right-hand side first, so its error wins over an absent assignment target. A print uses `to_string()` and `env.emit`. An expression statement discards its value.

The provided `exec_stmts` runs statements in order, stopping at an error or nonlocal `Flow`. This lesson returns `Flow::Normal`; future control flow uses the same return type. `run` creates an environment, parses and executes the program, then takes the buffered output.

Runtime failures stop execution at the failing statement. Earlier mutations are already made; this interpreter does not provide transactions or automatic rollback.
