---
title: Match IDS-style payload signatures
summary: Search packet payloads for byte patterns inside an offset and depth window, with optional case-insensitive matching.
minutes: 40
difficulty: hard
tags: security, detection, ids, network
id: sec-detection/signature-matching
mode: stdout
---
Intrusion detection systems started as pattern matchers: a signature says "this byte sequence, in this part of the payload". Good signatures are specific on purpose. A pattern that can appear anywhere matches harmless traffic constantly, so a signature also constrains where to look.

## Pattern, offset and depth

The offset skips bytes at the start of the payload, and the depth limits how many bytes after the offset may be searched (0 means to the end). Together they define a window `payload[offset .. offset+depth)` clipped to the payload length. The pattern must fit entirely inside that window. A match that starts inside but extends past the end of the window does not count, and an offset beyond the payload simply cannot match. Case-insensitive signatures fold ASCII letters on both sides before comparing; case-sensitive ones compare raw bytes.

Payloads arrive hex-encoded, two digits per byte, so decode them strictly: an odd number of digits or a non-hex character makes the payload invalid and it must be reported, not silently skipped, because an unparseable input is itself something a sensor should flag. You decoded hex carefully in the [hex lesson](/learn/sec-crypto/hex), and the signatures here deliberately describe only synthetic strings.

## Worked example

```text
SIG 1 0 0 case 474554          pattern "GET" anywhere
SIG 2 0 3 case 474554          "GET" within the first 3 bytes
SIG 3 4 0 nocase 41444d494e    "admin" (any case) from byte 4 on
SIG 4 0 0 case 5a5a            "ZZ"
(blank line)
474554202f41444d494e
504f5354202f61646d696e
4745
474554202f6
```

```text
payload 1: 1 2 3
payload 2: 3
payload 3: -
payload 4: invalid
```

Payload 1 is `GET /ADMIN`: signature 3 finds `ADMIN` from offset 4 with case folding. Payload 2 is `POST /admin`: `GET` is absent, but `admin` appears after offset 4. Payload 3 is the two bytes `GE`, too short for any pattern. Payload 4 has 11 hex digits, an odd count, so it is invalid and no signature is tried.

## Pitfalls

- **Window end.** The end is `min(payload length, offset + depth)` when the depth is non-zero. Use `start + pattern_length <= end` as the loop condition so the last pattern byte is inside the window.
- **Depth 0 is not a window of 0 bytes.** It means "to the end of the payload".
- **Offset past the end.** Do not underflow `size_t` arithmetic; just fail to match.
- **Folding both sides.** `nocase` folds payload and pattern; folding only one side makes uppercase patterns never match.
- **Order of output.** List matching signature ids in the order the signatures were defined, separated by single spaces.

## Why defenders care

A signature that is too broad drowns analysts, and one that is too narrow misses variants. Position and depth are how you tune that trade-off without rewriting the pattern. Cheap tricks for evading a naive signature (case changes, splitting a pattern across packets) are why real systems also reassemble streams; the [HTTP stream lesson](/learn/sec-packets/http-stream) shows that side of the problem.
