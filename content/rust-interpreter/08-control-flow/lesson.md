---
title: "Blocks, Branches and Loops"
summary: Add nested block scopes, boolean conditions and while loops to the tree-walking interpreter.
minutes: 50
difficulty: hard
tags: control-flow, scope, loops
id: rust-interpreter/control-flow
mode: harness
---
Builds on [Statements and Program Execution](/learn/rust-interpreter/statements).

Blocks group statements inside braces and create a child scope. `if` chooses one block; `while` repeats one. The statement dispatcher is already extended to call the functions you implement here.

`parse_block` requires `{`, parses statements until `}`, and returns their vector. A block may be empty. EOF before its closing brace must report the missing `RBrace`, not loop forever. `parse_if` consumes `if`, an expression, and a block, followed optionally by `else` and another block. There is no parenthesized-condition requirement and no special `else if` syntax. `parse_while` consumes `while`, its condition and a block.

Execution uses the same `Env` machinery as before. Each `exec_block` creates a fresh child and executes the statements there. Declarations stay inside that child; assignments can still reach enclosing bindings. Each iteration of a while loop gets a fresh block scope, so a local counter declared in the body starts fresh every time.

Conditions must evaluate to a boolean. An integer such as `1` is a type error even when another language might treat it as true. Evaluate only the selected branch. A while loop reevaluates its condition before every iteration, including the first; a false initial condition performs zero iterations.

`Flow` needs to travel through every layer. If a block produces a return, an enclosing branch or loop must return that flow immediately rather than continue executing. Functions arrive next, but building propagation now prevents their return behavior from needing a separate loop implementation.

The shared output buffer preserves print order through nested blocks. A scope's lifetime changes where variables are visible, not where printed lines are collected.
