---
title: "Compiling Expressions to Bytecode"
summary: Translate expression trees to postfix instructions and calculate their maximum stack depth.
minutes: 45
difficulty: medium
tags: bytecode, compiler, stack
id: rust-interpreter/compiling-expressions
mode: harness
---
Builds on [Review: A Persistent Session](/learn/rust-interpreter/review-session).

A tree-walking evaluator repeatedly follows enum nodes. A compiler can turn the same tree into a flat instruction sequence that a small machine executes later. The AST and type rules remain the same; only the execution representation changes.

The supplied `Op` enum is a **stack-machine instruction set**. `Const`, `Bool` and `Load` push a value. `Unary` transforms the top value. `Binary` pops the right operand, then the left operand, and pushes their result.

Compile operands before their operator: `1 + 2 * 3` becomes `Const(1), Const(2), Const(3), Binary(Mul), Binary(Add)`. This is postfix order. A recursive traversal mirrors the evaluator, except that it builds code instead of computing values. Compile the left child before the right child to preserve evaluation order.

Names compile to `Load(name)`; their values are resolved when the machine runs. Calls deliberately return `CompileError::Unsupported("calls")`: this second engine supports an expression subset while the original interpreter continues to support closures. Do not accidentally execute an expression during compilation or replace a variable with its current value.

`max_stack` predicts the largest number of values simultaneously present for valid straight-line expression code. A push increases depth by one. Unary leaves it unchanged. Binary consumes two and produces one, reducing depth by one. Track the high-water mark after each instruction; empty code requires zero slots.

`(1 + 2) * (3 + 4)` needs three slots, although its final result needs only one. Computing that bound is an early example of compiler analysis: reason about the effect of instructions without knowing their runtime values. The next lesson implements the machine that follows those effects.
