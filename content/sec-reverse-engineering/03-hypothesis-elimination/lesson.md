---
title: Eliminate hypotheses with evidence
summary: Test a list of candidate rules against observations, report which are refuted and why, and never confuse "fits so far" with "proven".
minutes: 40
difficulty: medium
tags: security, reverse-engineering, hypotheses, reporting
id: sec-reverse-engineering/hypothesis-elimination
mode: stdout
---
When you have several candidate explanations, the strongest move is not to look for support but to look for **refutation**. One observation that a hypothesis cannot produce kills it for good. An observation that it *can* produce proves nothing, because other hypotheses may produce it too. For example, the single probe `2 -> 4` is consistent with doubling, with squaring, and with adding two.

So the honest state of an investigation has three kinds of members: hypotheses that are **refuted** (and by which observation), hypotheses that are **consistent** with everything so far, and the gaps between them. If exactly one hypothesis survives you may report it, but as "fits all observations", not as "is the answer". If several survive, the correct next step is to pick the next probe that would separate them. If none survive, your list of candidates was wrong and you need new ideas.

In this lesson you build a small tool, `eliminate`, that does the bookkeeping. The candidates come from a fixed menu of functions on integers.

| name | meaning |
| --- | --- |
| `double` | `2 * x` |
| `square` | `x * x` |
| `plus_ten` | `x + 10` |
| `abs` | absolute value |
| `negate` | `-x` |
| `is_odd` | `1` if `x` is odd (including negative odd numbers), otherwise `0` |
| `sign` | `-1`, `0` or `1` |
| `clamp_100` | `x` limited to the range -100 to 100 |
| `mod_7` | the C remainder `x % 7` (negative for negative `x`) |
| `digit_sum` | sum of the decimal digits of the absolute value of `x` |

This is the habit behind every careful analysis report: say what you have eliminated and why, say what still stands, and say plainly that survival is not the same as proof. The [claims lesson](/learn/sec-reverse-engineering/claims-evidence) turns the same idea into a labelling scheme.
