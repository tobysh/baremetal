---
title: Validate redirect targets
summary: Allow only same-site relative paths after login, rejecting protocol-relative, backslash, control-character and encoded tricks.
minutes: 40
difficulty: medium
tags: security, auth, redirects
id: sec-appsec/safe-redirect
mode: harness
---
After login a site often sends the user on to a `next` address taken from the URL. If that address can be anything, an attacker mails a link such as `https://app.test/login?next=https://evil.test/` and the victim, who sees a trusted login page, lands on the attacker's page already "logged in" and primed to trust it. This is an open redirect, and the cure is to accept only local paths.

A local path starts with exactly one slash. The second character must not be another slash (`//evil.test` is protocol-relative and means a different host) nor a backslash (many browsers treat `\` as `/`). The encoded forms matter too: `/%2fevil.test` and `/%5cevil.test` decode to the same tricks. Reject any control character (below 0x20) or DEL, spaces, and any backslash, whether written raw or as a `%` escape, because browsers and proxies strip or reinterpret them (a tab inside `//` or a newline in a header is how response splitting starts). A malformed escape such as `%zz` or a truncated `%4` is also refused, and so is an empty or overlong value.

A whitelist of what you accept is always safer than a list of known bad strings. The login code of this very site follows the same rule: it keeps only a validated relative path, otherwise it falls back to a default page. Combine this with the checks from [path traversal](/learn/sec-c-bugs/path-traversal), which solves a similar problem for files.
