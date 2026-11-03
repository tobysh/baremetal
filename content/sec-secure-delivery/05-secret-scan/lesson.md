---
title: Find leaked secrets without leaking them again
summary: Scan synthetic secrets, skip reviewed placeholders, and report redacted previews and fingerprints.
minutes: 40
difficulty: medium
tags: secrets, scanning, supply-chain
id: sec-secure-delivery/secret-scan
mode: harness
---
Secrets end up in repositories, build logs and container layers all the time. Once a secret is committed, **deleting it later does not un-leak it**: copies exist in history, caches and clones. A scanner finds them early so the secret can be **revoked and rotated** (next lesson), which is the only real fix.

A scanner has two jobs that pull in opposite directions: find real secrets (few misses) and not cry wolf on placeholders (few false alarms), and it must be careful about its own output. A report that prints the secret it found copies it into CI logs, tickets and chat. So findings carry a **redacted preview** and a **fingerprint** (a hash prefix) that lets people refer to a secret and mark it as already handled without storing it.

All tokens here are invented. `stk_live_` / `stk_test_` tokens belong to a fictional service and unlock nothing.

## Rules

For each line (split on `\n`; a trailing `\r` is not part of any value), report, in this priority:

1. **`private-key`**: the line contains `-----BEGIN SYNTHETIC PRIVATE KEY-----`. The secret is that header string. Col is where it starts.
2. **`svc-token`**: `stk_live_` or `stk_test_` followed by exactly 24 ASCII letters or digits, not part of a longer run of letters, digits or underscores on either side (`\bstk_(?:live|test)_[A-Za-z0-9]{24}\b`).
3. **`assignment`**: a name containing `password`, `passwd`, `secret`, `api_key`/`api-key`/`apikey` or `token` (case-insensitive) followed by `:` or `=` (spaces allowed), then an optional quote, then a value of non-space characters other than quotes, commas and semicolons. The pattern is `(?i)(password|passwd|secret|api[_-]?key|token)[A-Za-z0-9_-]*\s*[:=]\s*["']?([^\s"',;]+)`. The value is reported only if it is at least 8 bytes long, has at least 6 distinct byte values, and is **not a placeholder**: it does not start with `${`, `$(`, `<`, `{{`, `%` or `env:`, and (case-insensitive) does not contain `example`, `placeholder`, `redacted` or `changeme`. If the value is itself a `svc-token` it is reported once, as `svc-token`.

`Col` is the 1-based byte column where the secret itself starts (for an assignment, the value, after any quote). Findings are sorted by line, then column. Several findings per line are possible.

## Output and the baseline

`Preview` and `Fingerprint` helpers are supplied. A secret whose fingerprint is in `baseline` has been triaged and is skipped. Never put the secret in any field of a `Finding`.

Limits: pattern scanners miss secrets with unknown formats, encoded secrets and anything split across lines, and the distinct-character rule is a crude stand-in for the entropy tests real tools use. A baseline hides a secret from the scanner, so it must be reviewed like code.
