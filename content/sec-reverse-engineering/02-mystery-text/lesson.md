---
title: Mystery text transformer
summary: Infer a string rule from examples, notice where the examples are silent, and pin down the threshold and the edge cases.
minutes: 35
difficulty: medium
tags: security, reverse-engineering, hypotheses
id: sec-reverse-engineering/mystery-text
mode: stdout
---
Text transformations are everywhere in samples: encoders, obfuscators, log scrubbers. A second synthetic program, `squeeze`, reads lines and prints one line for each. These are the first observations.

```
abc          ->  abc
hello        ->  hello
aaab         ->  a3b
xxxxx        ->  x5
ooops        ->  o3ps
```

The obvious theory is run-length encoding: replace a repeated character with the character and its count. But `hello` contains `ll` and comes out unchanged, and `abc` has no runs at all and does not turn into `a1b1c1`. So the rule must be conditional. Two candidate thresholds fit the first batch equally well: "runs of 3 or more are shortened" and, say, "runs of 4 or more", except that `aaab` and `ooops` have runs of exactly 3, which settles it in favour of three. Notice how much the choice of examples mattered; without `aaab` the two theories were indistinguishable.

More probes were run to test the edges.

```
(empty line)               ->  (empty line)
aab                        ->  aab
aabbbcc                    ->  aab3cc
zzzzzzzzzzzz               ->  z12
AAAaaa                     ->  A3a3
a    b                     ->  a 4b
```

Three new facts appear. The count is written in full, so twelve `z` become `z12`, not `z9z3`. Upper and lower case are different characters, so `AAAaaa` is two runs. And a space is a character like any other: four spaces between `a` and `b` become ` 4`. Inputs in this family only contain letters and spaces, which is why the digit counts are never ambiguous with the text. The lesson of this exercise is to list the questions your examples cannot answer (case, spaces, long counts, empty input) and make sure you have an observation for each one before you trust your rule.
