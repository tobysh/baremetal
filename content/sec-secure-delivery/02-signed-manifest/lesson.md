---
title: Verify a signed release manifest
summary: Check an Ed25519 signature before parsing, then match every listed file's size and digest and refuse missing, extra or oddly named files.
minutes: 40
difficulty: medium
tags: supply-chain, signatures, ed25519, manifest
id: sec-secure-delivery/signed-manifest
mode: harness
---
A digest is only as trustworthy as the place you got it from. A **signature** fixes that: the publisher signs a **manifest** (a list of file names, sizes and digests) with a private key, and you hold the matching public key from a channel you already trust. Now one signature covers every file, and the files can come from any mirror, because each is checked against the manifest.

This lesson uses `crypto/ed25519` from Go's standard library. Never write your own signature scheme. The keys in the tests are generated from fixed seeds and exist only for the exercises.

## The manifest

```
bm-manifest-v1
file app.bin 1204 sha256:<64 hex>
file readme.txt 88 sha256:<64 hex>
```

The signature covers the **exact bytes** of the manifest text. Do not re-encode, trim or normalise the bytes before verifying.

## Order matters: authenticate, then parse

Anything you read out of unauthenticated bytes is attacker-controlled. If you parse first, the parser itself becomes the attack surface for anyone who can reach you. So:

1. Verify the signature over the raw bytes. Failure is `ErrBadSignature`, and you stop. (`ed25519.Verify` **panics** if the public key has the wrong length; check it and return `ErrBadSignature`.)
2. Only then parse. A manifest that is correctly signed can still be malformed or hostile, because the signer may be buggy or compromised: that is `ErrBadManifest`.
3. Compare the files you actually hold against the manifest.

## Strict manifest rules

- First line exactly `bm-manifest-v1`; every line ends with `\n`, including the last; no blank lines.
- Each other line is `file <name> <size> sha256:<hex>` with single spaces.
- `size` is a canonical non-negative decimal (`0`, `17`, but not `+3`, `007`, `-1`).
- Names are plain file names: not empty, no `/`, no `\`, not `.` or `..`, no control characters or spaces. A signed `../../etc/x` is still a path traversal waiting for an installer.
- No name appears twice. Two entries for one name would let a verifier and an installer pick different ones.
- At least one file.

## Matching files

Each listed file must be present (`ErrMissingFile`), with the stated size and digest (`ErrFileMismatch`). A file you hold that the manifest does not list is `ErrExtraFile`: a signature that covers only some of a directory leaves room to slip an unlisted file in beside the signed ones.

Wrap errors with the file name for humans (`fmt.Errorf("%w: %s", ErrFileMismatch, name)`), but `errors.Is` must keep working.
