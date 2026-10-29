---
title: "Compiling Branches and Loops"
summary: Emit program bytecode, patch absolute jump targets and compare compiled execution with the interpreter.
minutes: 55
difficulty: hard
tags: compiler, control-flow, backpatching
id: rust-interpreter/compiling-control-flow
mode: harness
---
Builds on [Executing a Stack Virtual Machine](/learn/rust-interpreter/stack-vm).

The VM can execute jumps; the compiler now decides where those jumps go. `emit_stmt` appends instructions for a statement to an existing vector. Its indices are absolute, so every target must be measured from the beginning of that vector, including any instructions already present.

Simple statements compile their expression and append one operation: `Store` for declarations, `Assign` for assignment, `Print` for prints or `Pop` for discarded expression values. A block recursively emits its statements in order.

For `if`, compile the condition, append a placeholder `JumpIfFalse(0)`, and remember its index. Emit the then branch. Without an else, replace the placeholder with a jump to the current end. With an else, first append a placeholder unconditional jump over that else branch; patch the false jump to the else start, emit else, then patch the unconditional jump to the final end. This is **backpatching**: write a target after learning its address.

For `while`, remember the condition's start. Compile the condition and a false-jump placeholder, emit the body, then jump back to the saved start. Patch the false jump to the end after that back edge. Reevaluating the condition on every cycle preserves the interpreter's loop behavior.

This compiler supports globals in one flat scope. Unlike the tree interpreter's block scopes, declarations inside compiled blocks remain global. Equivalence comparisons must use programs that do not depend on shadowing or block-local visibility. Function definitions, calls and returns are also outside the compiled subset and must produce explicit unsupported errors.

Try compiling an empty block, a branch with no else, and a nested loop. Then compare `run` with `run_vm` for bounded programs in their shared subset. Matching results across two engines helps reveal wrong operand order, missing pops and misplaced jump targets.
