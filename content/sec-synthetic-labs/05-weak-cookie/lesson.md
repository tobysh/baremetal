---
title: Forge a cookie with a weak signing secret
summary: Recover a signing key from a small candidate list and forge a synthetic admin cookie.
minutes: 35
difficulty: medium
tags: security, synthetic-lab, weak-cookie
id: sec-synthetic-labs/weak-cookie
mode: harness
---

Signing gives integrity only while the secret stays secret. HMAC-SHA256 is a sound primitive, but a key drawn from a short public dictionary can be recovered offline using one known message and signature. The toy application signs a decoded body such as `user=guest&role=reader`; the cookie is base64url(body), a dot, then lowercase hexadecimal HMAC-SHA256(body).

The learner receives a known cookie, a small list containing its key, and the exact new body to sign. This isolates the weak-secret failure from parsing tricks. Secure systems generate a high-entropy key, store it outside source control, and rotate it if exposed. An authenticated cookie still needs server-side authorization and expiry checks.

Use Run to check compilation. Test supplies varied hidden inputs. All data and targets are fictional; nothing connects to an external service.
