---
title: Extend Without Breaking Callers
summary: Add quoted values and a Bool getter to a config parser while every existing file and call keeps parsing exactly as before.
minutes: 40
difficulty: hard
tags: compatibility, api, parsers
id: go-maintenance/compatibility
mode: harness
---
A parser's behaviour is its real interface. The signature says little; what callers depend on is what each input *means*, including the inputs nobody thought about. Add a feature that changes the meaning of an input that already exists, and some caller somewhere breaks.

The feature request here: config values should be able to be quoted so they can contain ` #` and keep leading spaces. The catch is in the existing parser. Today a value written `"hello"` is returned **with** its quotes, and `a = "x # y"` is cut at the ` #`. Some files in the wild depend on that.

A compatibility-safe approach:

- **Keep the old entry point byte-for-byte.** `Parse(src)` must give the same result for every input, including its quirks and its error messages.
- **Make the new behaviour opt-in.** A new entry point takes an options value whose **zero value means legacy behaviour**.
- **Adding methods is safe; changing a method's meaning is not.**
- **Prove it with old fixtures.** Pin a table of legacy inputs and outputs, written before the change.

## Honest limits

The tests pin a table of legacy inputs and the exported signatures. They cannot prove every possible legacy input is unchanged, which is why the safe design keeps the legacy code path unchanged instead of re-deriving it.
