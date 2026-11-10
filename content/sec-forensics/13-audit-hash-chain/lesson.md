---
title: Verify a tamper-evident log
summary: Check a synthetic hash-chained audit log, reporting the first malformed, out-of-sequence or altered entry, and see what chaining does and does not prove.
minutes: 40
difficulty: hard
tags: security, log-analysis, integrity
id: sec-forensics/audit-hash-chain
mode: stdout
---
## The forensic question

After an intrusion the attacker edits the audit log: they delete the line that shows a privilege grant, or change a username. Can you detect that? A **hash chain** makes tampering visible: each entry stores a hash that covers its own content and the previous entry's hash, so changing or removing any entry breaks every link after it.

## The data

Each line is `seq|actor|action|hash`. The hash is 16 lowercase hex digits of FNV-1a 64 over the text `PREV|SEQ|ACTOR|ACTION`, where `PREV` is the previous line's hash text, or sixteen zeros for the first entry:

```text
1|bob|login|c41643cfb47e9aec
2|dave|login failed|0ff91684c4b74bef
3|alice|create report-7|8f9f2732423d1f53
```

For line 1 the hashed text is `0000000000000000|1|bob|login`. For line 2 it is `c41643cfb47e9aec|2|dave|login failed`, which includes line 1's hash. That is the chain.

## Worked example

Now suppose an attacker deletes line 2 and leaves the rest:

```text
1|bob|login|c41643cfb47e9aec
3|alice|create report-7|8f9f2732423d1f53
```

Check line by line, stopping at the first problem, and in this order for each line:

1. **Malformed?** Exactly four `|`-separated fields; `seq` a positive number without leading zeros; actor and action non-empty; hash exactly 16 lowercase hex digits.
2. **Sequence.** Line 2 holds seq 3 but 2 was expected: `BROKEN at line 2: sequence (expected 2, found 3)`.
3. **Hash.** Only reached if the sequence is right: recompute the hash and compare.

If instead the attacker edits line 3's action and leaves the stored hash, line 3's recomputed hash differs: `BROKEN at line 3: hash mismatch`. If every line passes, print `chain ok: K entries head=H`, where `H` is the last hash (sixteen zeros for an empty log).

## Pitfalls

- **Order of checks.** A line with both a bad sequence and a bad hash reports the sequence; a malformed line reports `malformed` first.
- **Blank lines and extra bars** (`a|b|c|d|e`, or a `|` inside an action) are malformed: a smuggled separator would change what is hashed.
- **Leading zeros.** `01` is not the number 1 here.
- **Exact text.** The hash covers the stored previous hash text, not a recomputed one; format your own result with `%016llx` to compare.
- **Stop at the first failure**; later lines prove nothing once an earlier one is wrong.

## What a defender concludes

The first broken line marks where the evidence stops being consistent, not who changed it or why. FNV is not secret: an attacker who rewrites the file from the edit onwards produces a chain that still verifies. That is why the head hash must be stored somewhere the attacker cannot reach, such as another host or a signed checkpoint, and why real designs use a cryptographic or keyed hash. Chaining detects partial tampering; it does not prove the log is complete or truthful.
