---
title: Review: a request gatekeeper
summary: Combine session lifetimes, fixation refusal, CSRF origin and token checks and a safe redirect into one decision function.
minutes: 45
difficulty: medium
tags: security, auth, review
id: sec-appsec/review-request-gate
mode: stdout
kind: review
---
This review combines four defences into the single decision a web framework makes for every state-changing request. Each piece is already familiar: the session lifetimes and the refusal of reused identifiers from [session lifecycle](/learn/sec-appsec/session-lifecycle), the origin and token checks from [CSRF](/learn/sec-appsec/csrf-check), and the redirect rules from [safe redirects](/learn/sec-appsec/safe-redirect) (your `safe_redirect` is in the starter).

Order matters, and it should be cheapest and most fundamental first: does the session exist and is it still valid, then is the request from our own origin, then does it carry the right token, and only then do we build the response. Expired sessions are destroyed immediately, so repeated use of a dead identifier cannot revive it. On success the request refreshes the session's last-seen time. A bad `next` value does not fail the request; the response simply falls back to `/`, which is how login forms behave in practice.

Notice what is not new: every line is an idea you have already seen. The skill being trained is composing checks in the right order without letting one weaken another.
