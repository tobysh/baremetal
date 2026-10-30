---
title: Cookie and header audit
summary: Read canned HTTP responses and flag session cookies without Secure, HttpOnly or SameSite, and responses without HSTS or nosniff.
minutes: 25
difficulty: easy
tags: security, http-headers, cookies, hsts
id: sec-appsec/cookie-header-audit
mode: harness
---
Many web weaknesses are closed by a few response headers that the browser then enforces for you. Checking them is a good first review of any site, and it is simple enough to automate.

- **`Secure`** on a cookie means the browser only sends it over HTTPS, so it is never exposed on a plain-text connection.
- **`HttpOnly`** hides the cookie from page JavaScript, so a script injected by [cross-site scripting](/learn/ethical-hacking/output-encoding-xss) cannot simply read the session id.
- **`SameSite`** (`Lax`, `Strict` or `None`) limits when the browser attaches the cookie to cross-site requests, which blunts [CSRF](/learn/sec-appsec/csrf-check). `None` allows cross-site sending and is only accepted by browsers together with `Secure`.
- **`Strict-Transport-Security`** (HSTS) tells the browser to use HTTPS for this site for `max-age` seconds, so a downgrade to HTTP is refused. A very short age gives little protection.
- **`X-Content-Type-Options: nosniff`** stops the browser from guessing a content type and running an upload as a script.

Parsing is where the mistakes hide. Header and attribute names are case-insensitive. Attributes come after the first `;` of the cookie, so a cookie *named* `Secure` is not a `Secure` attribute. Headers end at the first blank line, and anything after it is the body, which could contain text that looks like a header. Your auditor must get these right, because a checker that is fooled by them gives false confidence. Cookie flags are part of [session lifecycle](/learn/sec-appsec/session-lifecycle) hygiene.
