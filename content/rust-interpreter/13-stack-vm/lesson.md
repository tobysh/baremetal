---
title: "Executing a Stack Virtual Machine"
summary: Run bytecode with a value stack, global variables, output and an instruction pointer.
minutes: 50
difficulty: hard
tags: vm, stack, bytecode
id: rust-interpreter/stack-vm
mode: harness
---
Builds on [Compiling Expressions to Bytecode](/learn/rust-interpreter/compiling-expressions).

The virtual machine executes the expression bytecode from the previous lesson and adds instructions needed by complete programs. `Vm` owns a value stack, a global variable map and an output vector. Its `run` method starts at instruction zero but retains those collections across calls.

Use an instruction pointer `ip` rather than a `for` loop. Fetch `code[ip]`, advance `ip`, then execute it. Normally execution moves forward once; a jump replaces `ip` with an absolute index. Reaching or passing the code length ends the run. Generated code uses valid targets, including exactly `code.len()` to exit.

Push literals, load globals and delegate operators to the existing checked arithmetic helpers. For binary operations pop the right operand first: a stack reverses the order of removal. Undefined globals return `Undefined(name)`. An empty pop returns the provided stack-underflow type error.

`Store` pops and defines or replaces a global. `Assign` pops and overwrites only an existing global. `Pop` discards a value; `Print` pops and records display text. `JumpIfFalse` always pops its condition, jumps only for false, and rejects non-booleans. This makes every branch consume its condition exactly once.

A loop can jump backwards to its condition. The bytecode is trusted compiler output for this course; arbitrary bytecode can contain infinite loops and no verifier is provided. Tests only run bounded programs.

The supplied `eval_via_vm` parses, compiles and executes an expression, returning its remaining stack value. It is now possible to compare two independently structured execution paths against the same language rules. In the next lesson, a compiler emits the jump and statement instructions this machine already understands.
