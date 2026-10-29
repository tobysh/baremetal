---
title: Verify a tamper-evident log
summary: Check a synthetic hash-chained audit log, reporting the first malformed, out-of-sequence or altered entry, and see what chaining does and does not prove.
minutes: 40
difficulty: hard
tags: security, log-analysis, integrity
id: sec-forensics/audit-hash-chain
mode: stdout
---
An attacker who gets onto a machine often edits the logs afterwards. A hash chain makes such edits detectable: each entry stores a hash that covers its own contents and the previous entry's hash. Changing or removing any earlier entry changes its hash, so every later link no longer matches. Sequence numbers add a second check, because a deleted entry leaves a gap that an investigator can see even before looking at hashes.

Your verifier reads entries `seq|actor|action|hash`, where the hash is the 16 digit lowercase hex of FNV-1a 64 over the text `PREV|SEQ|ACTOR|ACTION`. PREV is the previous entry's hash text, or sixteen zeros for the first entry. The FNV hash here is only a teaching device: anyone can recompute it, so an attacker who rewrites the whole file from the edit onwards produces a chain that verifies. Real designs use a keyed MAC or a cryptographic hash and, importantly, store the latest head hash somewhere the attacker cannot reach (a separate machine, a signed checkpoint) so a rewrite is noticed. Chaining detects partial tampering; it does not prove that the log was complete or truthful.

Report the first problem and stop. Check, in order, that the line is well formed, that its sequence number is the next expected one, and then that its hash matches. Observations stay observations: the first broken line shows where the evidence stops being consistent, not who did it.
