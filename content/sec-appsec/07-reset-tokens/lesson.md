---
title: Issue single-use reset tokens
summary: Model password reset tokens that expire, supersede older ones, can be used once and are not burned by a rejected new password.
minutes: 40
difficulty: medium
tags: security, auth, tokens
id: sec-appsec/reset-tokens
mode: stdout
---
Password reset links are bearer tokens that bypass the password, so they are as valuable as one. Safe handling follows a short checklist: the token expires quickly, it works once, and asking for a new one cancels the old one. If an old link stayed valid, every reset email ever sent would be a standing key to the account, sitting in mailboxes and logs.

Think of the lifecycle as a state machine per token: issued, then either used, superseded by a newer token for the same user, or expired. Check these in a fixed order so the audit log is unambiguous. A request with an unknown token is `invalid`. A token known but replaced is `superseded`. One already consumed is `already-used`. One older than its lifetime is `expired` (the age is `time - issued`; a token is expired when the age is greater than or equal to the TTL). Only then validate the new password. A rejected password must not consume the token, otherwise a typo locks the user out; but every other outcome leaves the state as it was.

What the user sees is different: the public page should give one generic message for all failures, so it cannot be used to probe which tokens exist. The detailed reasons here are for your own logs and tests. Tokens must be unguessable and compared safely as discussed in [constant-time comparison](/learn/sec-crypto/constant-time).
