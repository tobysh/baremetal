---
title: Session flaws and a safer session store
summary: Find predictable tokens, session fixation, missing expiry and missing logout in a toy session store, then write the store the way a defender would.
minutes: 40
difficulty: hard
tags: web, authentication, sessions
id: ethical-hacking/session-flaws
mode: harness
---
> **Simulated target.** Everything in this lesson is made up and runs inside the sandbox: the 'web app' is a C struct and a few functions, and the 'users' are test strings. In scope: the session store you write. No login page, cookie or server is touched. Never use a technique from this course on a system you do not own or do not have written permission to test. Start with [Law, ethics and permission](/learn/ethical-hacking/law-and-ethics) if you have not yet.

After a login, a web application gives the browser a **session token**. From then on, the token *is* the user. Whoever presents it is that user. Attackers therefore go after the token instead of the password, and every weakness in how tokens are made, kept and ended is a way in. The store you start with has four classic flaws:

1. **Predictable tokens.** It hands out `0000000000000001`, `0000000000000002` and so on. An attacker who logs in as themselves, sees a token ending in `...0007`, simply tries `...0006`. A token must come from a **cryptographically secure random source**. In this exercise the caller passes in `rnd`, a 64-bit value standing in for that source, and your token is its 16-digit lowercase hex form, zero padded. If a freshly generated token happens to collide with a live one, refuse to log in rather than overwrite someone else's session.
2. **Session fixation.** The attacker obtains a session id from the site *before* login, gets the victim to log in using it, then uses the same id. The defence is to **issue a new token at login** and invalidate the old one. `ss_login` gets the pre-login `old_token` (or `NULL`) and must kill it.
3. **No expiry.** A stolen or forgotten token should not live forever. Enforce an **idle timeout** (a session unused for more than 300 seconds is dead) and an **absolute lifetime** (more than 3600 seconds after creation is dead, however active). Expired sessions free their slot. A successful check refreshes the idle clock.
4. **Logout that does nothing.** If logout only deletes the browser's copy, a copied token keeps working. Logout must end the session on the **server**.

Also compare the **whole** token: a prefix such as `ab12` must not match. (Real code compares in constant time so timing leaks nothing; that cannot be observed in a test, but it matters.)

The table holds 8 sessions. If it is full after expired ones are purged, `ss_login` returns `NULL`. Think about who this can hurt: this is the kind of trade-off you should write into your findings, and you will practise the write-up in [the report lesson](/learn/ethical-hacking/finding-report).
