---
title: Check an artifact digest
summary: Parse an expected sha256 digest strictly, compare it with the bytes you received, and cap how much you are willing to read.
minutes: 25
difficulty: easy
tags: supply-chain, digest, sha256
id: sec-secure-delivery/artifact-digest
mode: harness
---
A **digest** is a short fingerprint of some bytes. If the bytes you received hash to the digest you were *told to expect*, they are the bytes the publisher meant, byte for byte. That is **integrity**. Be clear about what it does not give you:

- It says nothing about who published the bytes. A digest taken from the same untrusted server as the download only proves the server is consistent with itself. The expected digest must reach you by a channel you trust (a signed manifest, a pinned lockfile, a value you reviewed). Lesson 2 adds the signature.
- It says nothing about whether the bytes are *safe*. A digest of a bad program verifies perfectly.

## Strict parsing

Expected digests arrive as text, so the first job is deciding what text is acceptable. This course uses one canonical form:

```
sha256:<64 lowercase hexadecimal characters>
```

Anything else is `ErrDigestFormat`: other algorithms, upper case, surrounding spaces or a trailing newline, a wrong length, non-hex characters. Being strict matters because a lenient parser makes two people disagree about whether a lockfile line is the same digest, and because "close enough" is how truncated digests (`sha256:ba78`) slip through. A malformed expectation is a different failure from a **mismatch** (`ErrDigestMismatch`): the first means the policy is broken, the second means the artifact is wrong. Keep them distinct.

## Bounded reading

Artifacts are often read from a stream. A hostile server can send gigabytes, so decide the largest size you accept *before* you read, and fail with `ErrTooLarge` as soon as the stream has more than that, even if the first bytes would have matched. Read errors from the stream are not mismatches and must be returned as they are.
