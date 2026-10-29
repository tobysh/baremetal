---
title: "Evaluating the Expression Tree"
summary: Walk an AST to compute typed values and return explicit arithmetic and type errors.
minutes: 45
difficulty: medium
tags: evaluator, checked-arithmetic, result
id: rust-interpreter/evaluating-expressions
mode: harness
---
Builds on [Expressions with a Pratt Parser](/learn/rust-interpreter/pratt-parser).

An expression tree describes computation; an evaluator performs it. `eval` recursively returns `Result<Value, RuntimeError>`. At this stage values are integers and booleans. Literals are direct, while unary and binary nodes evaluate their children and delegate to `apply_unary` and `apply_binary`.

Always evaluate binary operands left to right. Propagating errors with `?` ensures the right operand is skipped if the left fails. This order will become observable when functions can print.

The language has strict types. `-` needs an integer; `!` needs a boolean. Arithmetic and ordering need two integers. Equality and inequality accept either two integers or two booleans, but mixing their kinds is a type error. There is no truthiness or implicit integer conversion.

Rust's machine arithmetic must not silently define the language. Use `checked_add`, `checked_sub`, `checked_mul`, `checked_neg`, `checked_div` and `checked_rem`, mapping `None` to `Overflow`. Check a zero divisor separately to report `DivByZero`. Integer division truncates toward zero; the remainder follows the dividend's sign. Even `i64::MIN / -1` and `i64::MIN % -1` must return overflow rather than panic.

Identifiers do not have an environment yet: return `Undefined(name)`. Calls return `NotCallable` without trying to execute them. Later lessons replace these cases as the runtime grows.

Notice the boundary between a language error and a Rust panic. Bad learner programs should produce a `RuntimeError` value that another layer can display. They should not abort the interpreter process. This is why checked arithmetic matters even when a debug Rust build would happen to panic on overflow.
