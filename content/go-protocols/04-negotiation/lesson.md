---
title: Version and Feature Negotiation
summary: Negotiate protocol versions and features, ignore unknown optional capabilities, and reject unmet requirements.
minutes: 40
difficulty: medium
tags: negotiation, compatibility, versioning
id: go-protocols/negotiation
mode: harness
---
Two programs on the same wire will not always be the same release. A protocol that cannot cope with that cannot be upgraded. Each side opens with a **Hello** that says what it can do:

| field | meaning |
|---|---|
| `MinVer`, `MaxVer` | the range of protocol versions it speaks (versions start at 1) |
| `Features` | bit set of optional features it supports |
| `Required` | bit set of features it will not proceed without |

## The rules

1. **Version:** the highest version in both ranges. No overlap is `ErrNoVersion`.
2. **Features:** a feature is enabled only if both sides offer it **and** the chosen version has it. The table `featureSince` says when each feature appeared (`FeatCompress` needs version 2, `FeatBatch` needs 3). A bit that is not in the table is unknown and is never enabled.
3. **Required:** if either side's `Required` bits are not all enabled, fail with `ErrMissingFeature`. An unknown required bit therefore always fails. Unknown *offered* bits are simply ignored; that is what lets a new peer talk to an old one.
4. A Hello whose `MinVer` is 0 or above `MaxVer` is `ErrBadHello` and checked first.

## Wire compatibility

Version 1 peers send a Hello of 6 bytes (`min`, `max`, `features` as big-endian u32). Later peers append `Required` (4 bytes). A decoder must accept the 6-byte form (`Required` = 0), and must ignore extra trailing bytes so that a future version can append more fields. Fewer than 6 bytes is `ErrBadHello`.

This is the "compatibility challenge" format: the hidden suite is exhaustive over small version ranges and compares you against an independent brute-force chooser. Whether your *design* of the feature table is wise is not machine-graded.
