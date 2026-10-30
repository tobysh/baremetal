---
title: Mystery function from input and output
summary: Infer what an unseen function does from a table of observations, then revise your theory when new observations contradict it.
minutes: 30
difficulty: medium
tags: security, reverse-engineering, hypotheses
id: sec-reverse-engineering/mystery-bits
mode: stdout
---
Sometimes you cannot read the code. You have a program, you can feed it inputs, and you can write down what comes out. The job is to work out the rule. This is the oldest reverse-engineering move there is, and the discipline is the same as in any science: propose a rule, test it against everything you have seen, and be ready to throw it away.

Here is a small synthetic program, `bitflip`, that reads unsigned numbers and prints one number for each. Nobody will show you its source. This is what the first batch of probes returned.

```
input   output
0       0
1       128
2       64
3       192
6       96
16      8
128     1
255     255
```

Look at the pairs in binary. `1` is `00000001` and the output `128` is `10000000`; `6` is `00000110` and `96` is `01100000`. Every output is its input with the bits **mirrored** inside one byte. That rule fits all eight observations, and it is tempting to stop there. But every input so far was below 256, so the data says nothing about what happens to larger numbers. A second batch was collected for exactly that reason.

```
input   output
256     0
257     128
1000    23
```

A rule that mirrors "the number" would have turned `256` into something huge. These results show that only the **low eight bits** take part and everything above them is thrown away: `1000` is `1111101000` in binary, its low byte is `11101000`, and mirrored that is `00010111`, which is 23. The first hypothesis was not wrong so much as incomplete, and only the counterexamples exposed it. Keep this habit: when you write down a rule, also write down which observations *could* have broken it and whether you actually have any.
