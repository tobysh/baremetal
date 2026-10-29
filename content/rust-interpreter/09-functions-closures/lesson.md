---
title: "Functions and Captured Environments"
summary: Implement calls, return propagation, recursion and mutable lexical closures.
minutes: 60
difficulty: hard
tags: functions, closures, lexical-scope
id: rust-interpreter/functions-closures
mode: harness
---
Builds on [Blocks, Branches and Loops](/learn/rust-interpreter/control-flow).

A function is now a runtime value containing its name, parameter names, body and defining environment. Calling it creates a child of that **captured** environment, binds the arguments locally and executes its body.

This is lexical scope: a function looks outward from where it was defined, not where it was called. A factory can return a nested function whose captured frame remains alive after the factory returns. Two factory calls allocate different frames, while two closures from one call share their captured mutations.

`exec_fn` creates a `Function` value capturing `env.clone()` and defines it under its name. Because the captured handle points to the live frame, inserting the function makes its own name available to recursive calls. Later definitions in that same frame are also visible, which permits mutual recursion.

`parse_fn` handles `fn name(a, b) { ... }`. Parameters require commas with no trailing comma. `parse_return` handles `return;` and `return expression;`. Both forms end with a semicolon.

`exec_return` wraps a value in `Flow::Return`; a bare return uses `Value::Nil`. Existing statement, branch and loop execution propagates that flow until `call_function` converts it into the call's result. Falling off the body also gives nil. In this small language, a top-level return ends the program.

For a call, evaluate the callee first and all arguments from left to right. Then check whether it is callable and whether the arity matches. Argument errors therefore precede arity errors. Bind parameters in a fresh child of the captured environment, run the body, and return the resulting value.

`Rc` keeps captured frames alive, but function values stored in their own frames can form reference cycles. This educational runtime does not reclaim those cycles; a production interpreter needs a garbage collector or another ownership strategy. The lesson focuses on correct lexical behavior.
