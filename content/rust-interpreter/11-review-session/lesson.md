---
title: "Review: A Persistent Session"
summary: Reuse the interpreter and diagnostics to build a session that preserves definitions and survives failed commands.
minutes: 40
difficulty: medium
tags: review, repl, state
id: rust-interpreter/review-session
mode: harness
kind: review
---
Builds on [Errors People Can Read](/learn/rust-interpreter/error-reporting).

A read-eval-print loop needs the same lexer, parser, evaluator and error reporter as a script runner. Its difference is lifetime: one `Session` owns an environment across multiple inputs. This review changes no language rules.

For each nonblank input, first try parsing it as one complete expression. If that succeeds, evaluate it and append its displayed value to the response. If expression parsing fails, treat the input as a complete program using `exec_source`. A semicolon therefore distinguishes `x + 1` (echo its value) from `x + 1;` (execute an expression statement and discard the value).

This fallback must happen only for parsing failures. If an expression parses but fails at runtime, report that failure once; do not try interpreting it as statements. The final error for a program comes from program parsing or execution, using the existing renderer.

Always take the environment's output buffer after the attempt. Prints from a function come before its returned expression value. Prints before a runtime error remain visible, followed by the diagnostic. Draining after every input prevents old output from appearing again on the next prompt.

Keep a count of inputs that complete successfully. Whitespace-only inputs return an empty response and do not count. Any other successful input counts once, even a comment-only input or a program containing several statements. Errors do not count. Earlier state changes made before a runtime error remain; syntax errors happen before execution starts.

`transcript` feeds a list of strings through one fresh session and records each prompt followed by its nonempty response. This makes the session behavior easy to test without terminal I/O, timing, or an interactive process.
