---
title: Combine selections with and, or and not
summary: Evaluate a rule condition such as `a and not ( b or c )` with correct precedence over named selections.
minutes: 40
difficulty: hard
tags: security, detection, parsing
id: sec-detection/boolean-conditions
mode: stdout
---
Real rules rarely have one test. They name several small selections (the parent is `java`, the image ends with `/nc`) and then combine them in a condition. Writing the combination is easy; evaluating it correctly is a small parsing problem, and a mistake changes what the rule fires on without any error. Nobody notices a rule that is silently too broad until the pager does, or too narrow until the incident review.

## Precedence

`not` binds tightest, then `and`, then `or`, and parentheses override all of them. So `a or b and c` means `a or (b and c)`, and `not a and b` means `(not a) and b`, which is a different rule from `not ( a and b )`. A selection name evaluates to true or false for the current event; a missing field makes the selection false (as in [rule matching](/learn/sec-detection/rule-matching)), and a `not` in front of it then makes the condition true.

A worked example. Selections `a` (user eq root), `b` (parent eq java), `c` (image endswith /nc), condition `a and not ( b or c )`:

```text
event 1: user=root parent=sshd image=/bin/ls     a=T b=F c=F  ->  T and not F  -> match
event 2: user=root parent=java image=/bin/ls     a=T b=T c=F  ->  T and not T  -> no match
event 3: user=bob  parent=sshd image=/bin/ls     a=F b=F c=F  ->  F and ...    -> no match
event 4: user=root image=/usr/bin/nc             a=T b=F c=T  ->  T and not T  -> no match
```

Event 4 has no `parent` field, so `b` is false rather than an error.

## Recursive descent

The grammar maps to one function per level, each calling the one below it:

```text
or      := and { "or" and }
and     := not { "and" not }
not     := "not" not | "(" or ")" | NAME
```

`parse_or` calls `parse_and` and keeps going while the next token is `or`; `parse_and` does the same with `parse_not`. `parse_not` handles a leading `not` (recursively, so `not not a` works), an opening parenthesis (which calls back into `parse_or`, then consumes `)`), or a selection name looked up in a table.

## Pitfalls

- **Wrong level order.** If `or` is below `and` in your call chain, you have swapped their precedence and `a or b and c` is wrong.
- **Short-circuiting.** `a and b` must still consume the tokens of `b` even when `a` is false; otherwise the parser loses its place in the expression. Parse first, combine after.
- **Evaluate per event.** Compute every selection for the event, then evaluate the condition; do not reuse values from the previous event.
- **Parentheses.** Forgetting to consume `)` leaves the parser one token behind for the rest of the expression.

## Why defenders care

Conditions are where rules get clever and where false positives are born. Keep them short, test them against events that should and should not match, and [tune](/learn/sec-detection/false-positive-tuning) them afterwards. The parsing exercise is to be exactly right about the ones engineers do write.
