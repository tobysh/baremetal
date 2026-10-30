---
title: Bind values into a query safely
summary: Build a query by substituting placeholders with quoted values: skip question marks inside literals, double quotes, check counts and bounds.
minutes: 50
difficulty: hard
tags: security, code-review, injection
id: sec-appsec/query-binding
mode: harness
---
SQL injection happens when data becomes part of the command: a name like `x' OR '1'='1` pasted into a query changes its structure. The real cure is a database driver that sends the statement and the values separately (prepared statements), so values can never be parsed as SQL. When you review code, look for string concatenation into queries. This lesson builds the tiny mechanism you would otherwise misuse, so that you understand what a placeholder must do.

The template contains `?` placeholders. Each is replaced by the next value in single quotes, with every quote inside the value doubled (`'` becomes `''`), the standard way to write a quote inside an SQL string. Two subtle points: a `?` inside a quoted literal in the template is plain text, not a placeholder (track whether you are inside quotes; a doubled quote `''` inside a literal toggles the state twice and so is handled by simply toggling at each quote), and the number of placeholders must equal the number of values, otherwise a value would be silently dropped or reused.

Finally, be a careful C writer: reserve room for the terminating NUL, never write past the buffer even when a value is huge, and leave `out[0] == 0` on failure so a caller cannot use a half-built query. These are the habits of [safe buffers](/learn/sec-c-bugs/review-safe-buffers). Remember that quote doubling is only a stand-in for real binding, and it does not help with identifiers, or with databases that treat backslash specially.
