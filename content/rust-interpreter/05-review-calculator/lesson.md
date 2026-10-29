---
title: "Review: A Resilient Calculator"
summary: Combine lexing, parsing and evaluation into a batch calculator that keeps going after bad lines.
minutes: 35
difficulty: medium
tags: review, evaluator, errors
id: rust-interpreter/review-calculator
mode: harness
kind: review
---
Builds on [Evaluating the Expression Tree](/learn/rust-interpreter/evaluating-expressions).

This review combines the lexer, stream, parser and evaluator into a usable calculator. The lower layers are supplied as working code; the new work is orchestration using `Result`, iteration and checked arithmetic you already used.

`calc` accepts one expression and composes the existing parser with the evaluator. `Error` distinguishes parse failures from runtime failures, with `From` implementations so `?` can lift either one automatically.

Batch input needs a more helpful policy than returning on its first error. `calc_lines` processes each nonblank line independently. A malformed expression produces an error response for that line, and later lines still run. Number lines from one using their original position in the input; blank lines produce no response but still count toward numbering.

Keep parsing and runtime messages distinct. A parser reports a byte position within that line and a syntax message. A runtime error already has a `Display` implementation. These operations belong in the adapter layer; changing the parser to know about batch line numbers would make it harder to reuse.

`calc_total` answers a different question: sum only successful integer results. Ignore errors and booleans. Return `None` if there are no successful integers, or if adding one would overflow. A successful zero is still a result and must yield `Some(0)` rather than `None`.

Try input containing valid arithmetic, whitespace, a comparison and a division by zero. The calculator should preserve every line's independent outcome. No parser or evaluator rule changes in this review.
