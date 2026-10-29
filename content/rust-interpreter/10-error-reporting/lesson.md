---
title: "Errors People Can Read"
summary: Convert byte offsets into line and character columns, source excerpts and caret diagnostics.
minutes: 40
difficulty: medium
tags: diagnostics, unicode, errors
id: rust-interpreter/error-reporting
mode: harness
---
Builds on [Functions and Captured Environments](/learn/rust-interpreter/functions-closures).

A parser's byte offset is useful for machines, but people need a line and column. `line_col` converts an offset into a one-based pair. Scan `char_indices()` up to the requested byte position: a newline increments the line and resets the column to one; every other character increments the column. Positions at or beyond EOF clamp to the end.

Columns count Unicode scalar values here, so a multibyte character occupies one column. This is more useful than counting bytes, although tabs, combining marks and wide characters can still make a terminal's visual caret differ. Inputs use byte boundaries from the lexer and parser.

`render_error` distinguishes syntax and runtime failures. Syntax errors contain positions, so display the message, line/column, offending source line and a caret. The gutter must widen for multi-digit line numbers. For example, an error at column three on line twelve has a `12 |` source gutter and a two-space number gutter on the caret line.

Runtime errors currently carry a kind and message, without source spans. Render them as `runtime error: TEXT`; do not fabricate a position. Attaching spans to expression and statement nodes would be a separate extension to the AST and evaluator.

`report` ties this together: run a complete program, join its successful output with newlines, or render its returned error. On failure it returns the diagnostic, with no trailing newline. It does not return the output printed before the failure; a persistent session will use the lower-level output buffer directly in the next review.

Keeping error data separate from its presentation lets tests inspect precise values and lets different frontends choose their own display. The parser still returns `ParseError`; only this outer adapter knows about source gutters.
