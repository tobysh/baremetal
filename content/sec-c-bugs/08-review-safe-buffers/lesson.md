---
title: Review: a defensive record copy
summary: Combine signed-length validation, safe size arithmetic, bounded copying and literal data handling.
minutes: 25
difficulty: medium
tags: security, defensive-programming
id: sec-c-bugs/review-safe-buffers
mode: harness
kind: review
---
This review combines the checks from [signed lengths](/learn/sec-c-bugs/signed-unsigned), [allocation overflow](/learn/sec-c-bugs/size-overflow) and [bounded copying](/learn/sec-c-bugs/unbounded-copy). A record arrives as a byte span plus a signed count. The caller wants its bytes followed by a zero byte.

Validate the sign before converting to `size_t`. Next reserve a terminator without computing `n + 1` too early: requiring `n < cap` proves both that the addition is representable and that the destination is large enough. Positive data lengths require a readable source, and every successful call requires a destination because even an empty result stores a terminator.

The record is bytes, not a C string, so an embedded zero is part of the payload and must not stop the copy. Percent characters also stay literal data. Validate everything before writing, then copy exactly the accepted count and append the terminator. Failures leave the destination unchanged. This is the same sequence used by more complicated parsers: establish the contract, prove the bounds, perform the write.
