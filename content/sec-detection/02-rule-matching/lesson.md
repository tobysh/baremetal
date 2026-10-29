---
title: Evaluate Sigma-style rules
summary: Load simple field rules (equals, contains, starts with, ends with) and report which rules each event triggers.
minutes: 30
difficulty: medium
tags: security, detection, rules
id: sec-detection/rule-matching
mode: stdout
---
A detection rule is data, not code. Community rule formats describe a match as a field, an operator and a value, so analysts can share and review rules without recompiling a sensor. Your engine reads rules first, then streams events through them.

Operators are case-sensitive here. `eq` compares the whole value, `contains` looks for a substring, `startswith` and `endswith` compare the ends. A rule never matches an event that lacks the field: a missing field is not an empty string, and a rule that treats it as one can fire on the wrong kind of event. Remember that a longer string can contain the pattern at its very end, so `endswith` needs a length check before comparing.

Reuse the field extractor from [the previous lesson](/learn/sec-detection/field-extraction) (it is included in your starter). Good rule engines are boring on purpose; the value is in how clear the rules are.
