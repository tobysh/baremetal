---
title: Review: evidence triage
summary: Combine signatures, a short hex preview and a toy checksum into a reproducible evidence observation.
minutes: 25
difficulty: medium
tags: security, defensive-programming
id: sec-forensics/review-evidence-triage
mode: harness
kind: review
---
This review combines [signatures](/learn/sec-forensics/file-signatures), [hex display](/learn/sec-forensics/hex-dump) and [manifest checks](/learn/sec-forensics/hash-verification). No new file format is introduced. Produce one compact observation for each item without relying on its extension.

Use only PNG, JPEG, PDF and ZIP signatures in this review; other formats are unknown. Display at most the first eight bytes, lowercase hex separated by single spaces, or a dash when there are no bytes. Calculate the same 64-bit FNV checksum as before and compare it with the supplied reference.

Report OK or CHANGED relative to that reference. Do not turn OK into a claim of authenticity: this is still the deliberately noncryptographic teaching checksum. Likewise, recognizing a signature does not prove a complete valid file. Keeping those observations separate makes the report reproducible and keeps conclusions proportional to the evidence. The output is bounded and always terminated for a positive capacity, so even a reporting path follows the same defensive rules as the parser.
