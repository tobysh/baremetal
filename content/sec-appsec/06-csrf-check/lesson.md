---
title: Decide whether to trust a state-changing request
summary: Apply cross-site request forgery defences: method rules, session lookup, exact origin matching and a per-session token.
minutes: 40
difficulty: medium
tags: security, auth, csrf
id: sec-appsec/csrf-check
mode: stdout
---
Browsers attach cookies automatically, so a malicious page can make your browser send a request to a site where you are logged in, and the site cannot tell it from a real click. This is cross-site request forgery (CSRF). The defences work by requiring something the attacker's page cannot supply.

First, use methods properly. `GET`, `HEAD` and `OPTIONS` must not change state; if a URL such as `/profile/delete` or `/transfer` can be triggered by `GET`, any image tag can fire it, so refuse it. The other methods (`POST`, `PUT`, `PATCH`, `DELETE`) change state and need the real checks. Second, the request must belong to a valid session. Third, the browser reports where the request came from in the `Origin` header; compare it to the site's own origin as an exact string. Prefix or substring checks are wrong (`https://app.test.evil.test` starts with the site's name) and so is a trailing slash. A literal `null` origin and a different scheme are cross-origin. A missing origin is allowed to continue here, because some legitimate clients omit it, but the token check below is mandatory either way. Fourth, the request must carry the per-session anti-forgery token in its body, exactly equal to the one stored for the session.

None of this repairs XSS: script on your own pages can read the token. Stored data and escaping are covered in [the code review lessons](/learn/sec-appsec/query-binding). The token is generated just like the one in [the crypto token lesson](/learn/sec-crypto/review-token-bytes).
