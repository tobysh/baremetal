---
title: Scan C source for banned functions
summary: Write a small lexer-aware linter that finds calls to dangerous functions while ignoring comments, strings, look-alike names and struct members.
minutes: 50
difficulty: hard
tags: security, code-review, static-analysis
id: sec-appsec/banned-calls
mode: stdout
---
Secure code review often starts with a tool, because some mistakes are easy to find mechanically. Functions such as `gets` (no length at all), `strcpy`, `strcat`, `sprintf` (no bound) and `system`/`popen` (a shell parses the command) have well-known safer replacements. A banned-function check in the build catches them before review. You met their consequences in [unbounded copy](/learn/sec-c-bugs/unbounded-copy) and [format strings](/learn/sec-c-bugs/format-strings).

A naive `grep strcpy` fails in both directions. It reports mentions in comments and strings (noise that teaches people to ignore the tool) and it reports `my_strcpy` and `strcpy_s`, which are different functions. A useful scanner is a tiny lexer: walk the text with a state (code, line comment, block comment which may span lines, string literal, character literal), honour backslash escapes inside literals, read whole identifiers, and decide a call when a banned identifier is followed (after spaces or tabs) by an opening parenthesis. A name used as a member (`cfg.system(` or `p->system(`) is not the library function either.

Static scanners find patterns, not bugs: they cannot see that a `strcpy` is bounded by earlier code, and they miss calls through function pointers. Treat the output as a worklist for a human reviewer.
