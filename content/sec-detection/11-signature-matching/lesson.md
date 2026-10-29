---
title: Match IDS-style payload signatures
summary: Search packet payloads for byte patterns inside an offset and depth window, with optional case-insensitive matching.
minutes: 40
difficulty: hard
tags: security, detection, ids, network
id: sec-detection/signature-matching
mode: stdout
---
Intrusion detection systems started as pattern matchers: a signature says "this byte sequence, in this part of the payload". Good signatures are specific on purpose. A pattern that can appear anywhere matches harmless traffic constantly, so a signature also constrains where to look. The offset skips bytes at the start of the payload, and the depth limits how many bytes after the offset may be searched (0 means to the end).

The pattern must fit entirely inside the window. A match that starts inside but extends past the end of the window does not count, and an offset beyond the payload simply cannot match. Case-insensitive signatures fold ASCII letters on both sides before comparing. Payloads arrive hex-encoded, so decode them strictly: an odd number of digits or a non-hex character makes the payload invalid and it must be reported, not silently skipped, because an unparseable input is itself something a sensor should flag. You decoded hex carefully in the [hex lesson](/learn/sec-crypto/hex), and the signatures here deliberately describe only synthetic strings.

Cheap tricks for evading a naive signature (case changes, splitting a pattern across packets) are why real systems also reassemble streams; the [HTTP stream lesson](/learn/sec-packets/http-stream) shows that side of the problem.
