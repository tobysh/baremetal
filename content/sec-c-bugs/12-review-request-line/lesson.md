---
title: Review: validate a request line
summary: Combine strict token checks, segment-aware traversal rejection and bounded copying in one request-line parser.
minutes: 30
difficulty: medium
tags: security, parsing, input-validation
id: sec-c-bugs/review-request-line
mode: harness
kind: review
---
This review combines [parsing records without trusting lengths](/learn/sec-c-bugs/tlv-parser), [contained paths](/learn/sec-c-bugs/path-traversal) and [truncation detection](/learn/sec-c-bugs/truncated-output). A toy server receives one request line such as `GET /docs/a.txt HTTP/1.1` and must either accept it or reject it completely.

Be strict: an allowlist of methods, exactly one space between the three fields, a version that is exactly one of two strings, and a target made only of printable non-space ASCII that starts with `/`. Reject a target that has a `..` segment, but not a name that merely contains dots such as `/a/..b`. Both outputs must fit with their terminating zeros, and a rejected line must leave both outputs empty so later code cannot act on half-parsed data.

There is no new concept here. Notice how the order of checks matters less than checking everything before any success is reported, and how few lines the safe version needs when each rule is a separate, readable step.
