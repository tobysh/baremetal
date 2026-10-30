---
title: Browser security concept quiz
summary: Explain CSRF, CORS and session decisions using a table of browser scenarios.
minutes: 35
difficulty: medium
tags: security, synthetic-lab, browser-policy-quiz
id: sec-synthetic-labs/browser-policy-quiz
mode: harness
---

This is an explanation-and-quiz lesson, not an attack against a live browser. Each question has a fixed answer label. Read the policy carefully before filling the answer table.

CSRF abuses ambient credentials: a browser may attach a session cookie to a request the user did not intend. A valid unpredictable CSRF token tied to the session and checked by the server protects state-changing operations; a missing token should cause rejection. SameSite adds protection but is not a complete replacement in every application.

CORS controls whether JavaScript can read a cross-origin response. Blocking that read does not necessarily block the request or its side effect. Credentialed CORS cannot use `Access-Control-Allow-Origin: *` to grant the requesting script access. An explicit allowed origin with `Access-Control-Allow-Credentials: true` can permit access when the browser sends credentials.

A server should rotate the session identifier after login or a privilege change. HttpOnly prevents scripts reading a cookie, but scripts executing in the origin can still make authenticated requests. Logout should invalidate the server-side session, not merely remove one browser's cookie.

Use Run to check compilation. Test supplies varied hidden inputs. All data and targets are fictional; nothing connects to an external service.
