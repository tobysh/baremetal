---
title: Review a lockfile change
summary: Compare two dependency lockfiles and report downgrades, swapped hashes, off-policy sources and look-alike package names, with a severity for each.
minutes: 45
difficulty: medium
tags: supply-chain, dependencies, lockfile, code-review
id: sec-secure-delivery/lockfile-review
mode: harness
---
A **lockfile** pins every dependency to an exact version, a content digest and a source, so two builds get the same bytes. A change to the lockfile is therefore a change to the code you ship, and it deserves review even when no source file changed. Reviewers rarely read hundreds of lines of hashes, so a tool should read them and flag the changes that matter.

## The synthetic format

```
# bm-lock v1
pkg <name> <MAJOR.MINOR.PATCH> sha256:<64 hex> <source-host>
```

All names, hosts and digests in this course are invented (`registry.example.test`). `ParseLock` is supplied: it is strict (comments and blank lines allowed, one package per name, plain three-part versions without leading zeros, lowercase hex).

## What a reviewer looks for

`Review(oldText, newText, policy)` returns `[]Finding{Pkg, Kind, Severity, Detail}` using these kinds:

| Kind | Meaning | Severity |
| --- | --- | --- |
| `added` | package in new, not in old | `info` |
| `removed` | in old, not in new | `info` |
| `upgrade` | same name, higher version | `info` |
| `downgrade` | same name, lower version. Old versions have known bugs, and forcing one is a classic attack. | `high` |
| `hash-changed` | **same version**, different digest. A released version must never change; this is either a republished (tampered) artifact or a poisoned mirror. | `critical` |
| `source-changed` | same name, different source host | `high` |
| `source-not-allowed` | a package in the new lockfile whose source is not in `policy.AllowedSources` (reported for every such package, changed or not) | `high` |
| `lookalike` | an *added* package whose name is confusable with another name (see below) | `high` |

One package can produce several findings. A downgrade with a changed hash yields `downgrade` only, since the version differs; a changed digest is only suspicious when the version stays the same.

### Look-alike names (typosquatting)

An attacker publishes `reqeusts` hoping someone types it. For an added package `n`, compare it with every other name `m` in the old lockfile and in `policy.Popular`; `n` is a look-alike of `m` if `n != m` and either they are equal after lowercasing and mapping `_` to `-`, or their Levenshtein edit distance is exactly 1 and both names are at least 4 characters long (short names would match too much). Report one `lookalike` finding per added package, with the first match in alphabetical order of `m` in `Detail` (as `resembles <m>`). A name that is *already* in the old lockfile is never a look-alike of itself.

## Output

Sorted by package name, then by the kind order in the table above. `Detail` is free text except for `lookalike`. Return the parse error (wrapped is fine) if either lockfile is malformed.

This is a **heuristic**: it will miss attacks (a malicious update that keeps its hash consistent and its version higher) and it will flag some innocent changes. Tooling narrows what a human must read; it does not replace reading.
