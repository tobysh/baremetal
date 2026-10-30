---
draft: true
title: Parse a versioned embedded configuration record
summary: Decode and encode a tagged, versioned config blob with optional fields, defaults and unknown-field rules, and keep its token out of logs.
minutes: 50
difficulty: hard
tags: security, toy-maldev, configuration, versioning
id: sec-toy-maldev/embedded-configuration
mode: harness
---

This course constructs **inert payload data** for an invented machine. Payload instructions can emit text, read explicitly supplied map entries or advance virtual time. The toy machine has no instruction for host execution, network access, process injection or filesystem mutation. Everything stays inside the existing Go sandbox.

Samples often carry a small configuration blob: a label for the campaign, an interval, a list of allowed items. Analysts extract these records because they describe intent. Developers of legitimate agents, installers and firmware also embed configuration, so the format questions are the same: what happens with a truncated record, a field this build has never heard of, a missing optional field, or a record written by a newer version?

This lesson builds the reader and writer for an invented record called `BCFG`. It is a lab-only configuration for the toy loader; nothing in it addresses a real host.

## Layout

| Offset | Size | Field |
| --- | --- | --- |
| 0 | 4 | ASCII magic `BCFG` |
| 4 | 1 | format version (1 or 2) |
| 5 | 2 | body length, little-endian uint16 |
| 7 | length | body: a sequence of fields |

A field is one tag byte, one length byte and that many value bytes. Known tags:

| Tag | Since | Meaning | Value |
| --- | --- | --- | --- |
| 1 | v1 | label (required) | 1–32 bytes |
| 2 | v1 | interval in virtual ticks (optional, default 10) | uint16 LE, nonzero |
| 3 | v1 | operation budget (optional, default 64) | uint16 LE, 1–4096 |
| 4 | v2 | allowed capability keys (optional, default none) | count byte (0–8), then count entries of a length byte (1–32) and key bytes; keys unique |
| 5 | v2 | access token (optional, default none) | 1–64 bytes |

A **tag below 0x80 that this version does not know is critical**: the reader cannot know what it means, so it must reject the record. A **tag of 0x80 or more is ignorable**: skip its value and carry on. This is the same compatibility idea used by PNG chunks and TLS extensions. A tag introduced in version 2 appearing in a version-1 record is "unknown to version 1", so it is rejected. A version greater than 2 is rejected as unsupported.

Forward compatibility has a cost. Ignored fields are dropped when a record is read and written back, so a rewriter must not claim it preserved them.

## Decoding

Decode returns a Config with defaults applied. All lengths are untrusted: the body length must match the bytes present exactly, and a field must fit inside the body. A truncated record is an error at every possible cut point. Duplicate known tags and missing label are errors, and so are invalid values (zero interval, budget over 4096, duplicate keys).

## Encoding

Encode validates the same rules and writes the **lowest version that can express the Config**: version 1 when there is no Allow list and no Token, otherwise 2. Fields equal to their default are omitted, and fields are written in ascending tag order, so the same Config always produces the same bytes. Decode of Encode's output must return the same Config.

## Encoding is not protection

The token field is stored as plain bytes. Hex, base64, XOR with a key written next to the data or any other reversible transform would be a different spelling of the same bytes: anyone holding the record can reverse it. If a secret must be recorded, the real answers are not to ship it in a distributable artifact at all, or to keep it behind a boundary the holder of the artifact cannot cross. Neither can be shown by a byte format. What this lesson can show is the smaller discipline that follows: a value you must handle should never appear in logs, error messages or describing output, and a test can check that.

`Describe` produces a one-line summary for logs. It may show that a token exists and how long it is, but must not include the token or any simple re-encoding of it.

The token in these tests is a made-up lab string. A length is still a small leak, and the lesson does not claim otherwise.

Run uses a small public example in main. Test exercises fresh values and malformed inputs. Keep the provided API names so the harness can call your functions.
