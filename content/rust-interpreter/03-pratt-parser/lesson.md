---
title: "Expressions with a Pratt Parser"
summary: Build expression trees using binding powers, prefix operators and function-call suffixes.
minutes: 50
difficulty: hard
tags: parser, pratt, syntax-tree
id: rust-interpreter/pratt-parser
mode: harness
---
Builds on [A Peekable Token Stream](/learn/rust-interpreter/token-stream).

Parsing turns a token stream into an **abstract syntax tree** (AST). The supplied `Expr` enum describes literals, names, unary operators, binary operators and calls. `Box<Expr>` lets a recursive enum have a finite size.

A Pratt parser uses binding powers to handle precedence with one recursive function. `parse_expr(stream, min_bp)` first parses something that can start an expression: a literal, identifier, parenthesized expression, or prefix `-`/`!`. This becomes its current left-hand side.

Then inspect the next token. The supplied `infix` maps binary operators to `(left_power, right_power, op)`. If the left power is below `min_bp`, this invocation stops without consuming the operator. Otherwise consume it, recursively parse the right side at `right_power`, and combine both sides into a new left-hand tree.

The right power is one higher than the left power, making these operators left-associative. Thus `10 - 4 - 3` becomes `(- (- 10 4) 3)`. Multiplication has higher power than addition, so `1 + 2 * 3` becomes `(+ 1 (* 2 3))`. Comparisons share the lowest power. Prefix operators parse their operand at `PREFIX_BP`; calls bind tighter still at `CALL_BP`.

A call is a suffix, so the callee can be any expression: `outer(2)(3)` is a call whose callee is another call. Consume `(`, parse zero or more comma-separated expressions, require `)`, and wrap the existing left-hand side in `Expr::Call`.

`parse_expression` already checks that nothing remains after one expression. Inside a call, comma and right parenthesis naturally stop the recursive parser because they are not infix operators. Position errors at the unexpected token, or EOF. The supplied `show` prints trees so you can inspect precedence before an evaluator exists.
