---
title: Combine selections with and, or and not
summary: Evaluate a rule condition such as `a and not ( b or c )` with correct precedence over named selections.
minutes: 40
difficulty: hard
tags: security, detection, parsing
id: sec-detection/boolean-conditions
mode: stdout
---
Real rules rarely have one test. They name several small selections (the parent is `java`, the image ends with `/nc`) and then combine them in a condition. Writing the combination is easy; evaluating it correctly is a small parsing problem, and a mistake changes what the rule fires on without any error.

Precedence matters: `not` binds tightest, then `and`, then `or`, and parentheses override all of them. So `a or b and c` means `a or (b and c)`. A recursive descent parser has one function per level (or, and, not, primary), each calling the one below it, and it maps directly onto that grammar. A selection name evaluates to true or false for the current event; a missing field makes the selection false (as in [rule matching](/learn/sec-detection/rule-matching)).

Rules that are hard to read are hard to tune, so engineers keep conditions short. The parsing exercise is to be exactly right about the ones they do write.
