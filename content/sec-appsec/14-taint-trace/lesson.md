---
title: Trace tainted data to dangerous sinks
summary: Follow untrusted values through assignments, concatenation and sanitisers in a toy language and report which dangerous calls receive them.
minutes: 55
difficulty: hard
tags: security, code-review, taint
id: sec-appsec/taint-trace
mode: stdout
---
Many injection bugs share one shape: data from an untrusted source (a request parameter, an environment variable) flows through the program and reaches a dangerous sink (a query, a shell command) without being made safe on the way. Reviewers trace that path by eye; static analysers do it with taint tracking. You will write a miniature one for a straight-line toy language.

Each variable has a taint mask with one bit per kind of danger: bit 1 means unsafe for SQL, bit 2 unsafe for a shell. Sources (`input()`, `env()`) produce both bits. A string literal has none. Concatenation `a + b` carries the union of its operands. A sanitiser clears only the bit it really handles: `escape_sql(x)` clears the SQL bit, `quote_shell(x)` the shell bit. Applying the wrong sanitiser leaves the value dangerous, which is a common real-world bug and the point of tracking bits separately. Any other function call is treated conservatively: its result carries the union of its argument's taint, since you cannot prove it is safe. Assignment replaces the target's mask, and a variable never assigned is clean.

Straight-line analysis ignores branches and loops, so it finds the easy cases; real analysers handle control flow, aliases and fields, and still produce false positives and misses. It pairs well with the [banned-call scanner](/learn/sec-appsec/banned-calls) and with [safe query binding](/learn/sec-appsec/query-binding), which removes the need for escaping at all.
