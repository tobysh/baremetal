---
title: Claims, evidence and assumptions
summary: Label each finding as confirmed, supported, assumption, disputed or refuted from the kinds of evidence behind it.
minutes: 40
difficulty: medium
tags: security, reverse-engineering, reporting
id: sec-reverse-engineering/claims-evidence
mode: stdout
---
A report is only as useful as the reader's ability to tell what you *know* from what you *guess*. "The sample beacons every 60 seconds" can mean three very different things: you read the interval out of the code, you watched it happen, or an old blog post said so. The words are the same; the weight is not. Good analysts attach evidence to every claim and label the strength honestly.

In this lesson each piece of evidence has a **kind**:

- `static`: read from the sample itself (a constant, a string, a code path).
- `dynamic`: observed when the sample ran in the lab.
- `report`: someone else's statement that you have not checked yourself.

and a **stance**, either `supports` or `contradicts`. Static and dynamic evidence are first-hand: you can reproduce them. A report is second-hand, however respectable. Two independent first-hand kinds agreeing is the strongest case you can make, because code can say one thing and behaviour another (a path that never runs, or a value changed at run time).

The labels follow fixed rules, applied in this order:

1. A contradiction from `static` or `dynamic` evidence makes the claim `refuted`, even if other evidence supports it. Something you saw directly beats everything else.
2. Otherwise a contradicting `report` makes it `disputed`: someone disagrees and you have not settled it.
3. Otherwise support from both `static` and `dynamic` makes it `confirmed`; support from only one of them makes it `supported (static)` or `supported (dynamic)`. Repeating the same kind several times counts once; ten static clues are still one kind of evidence.
4. Everything else, including a claim backed only by reports, or by nothing, is an `assumption`. Assumptions are not bad, but they must be visible as assumptions.

This is the difference between a lead and a finding, and it keeps a report from quietly promoting a guess. The reasoning ties back to [hypothesis elimination](/learn/sec-reverse-engineering/hypothesis-elimination): a single first-hand contradiction outweighs any amount of agreement.
