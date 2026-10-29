---
title: Outbound request policy (SSRF)
summary: A feature that fetches a user-supplied URL can be steered at internal systems. Write the allow-list, resolver and redirect checks that stop it.
minutes: 45
difficulty: hard
tags: web, ssrf, validation, secure-coding
id: ethical-hacking/ssrf-policy
mode: harness
---
> **Simulated target.** Everything in this lesson is made up and runs inside the sandbox: the 'URLs' are strings, the 'DNS resolver' is a function the tests hand to your code, and every host ends in .test. In scope: the policy function you write. Nothing connects anywhere. Never use a technique from this course on a system you do not own or do not have written permission to test. Start with [Law, ethics and permission](/learn/ethical-hacking/law-and-ethics) if you have not yet.

Many applications fetch a URL for the user: link previews, image imports, webhooks. That feature runs **inside** the server's network, with the server's access. **Server-side request forgery (SSRF)** is steering it at something the user could never reach directly, such as an internal admin page, a monitoring port or a metadata service on a link-local address, with the server making the request on the attacker's behalf. The attacker never touches the target; the application does, and the answer may even come back in the preview.

A blocklist ("reject `localhost`") loses, because there are too many spellings of the same place. The defender's approach is an **allow-list** plus checks on what the name *really* leads to:

1. Only `http` and `https` (any letter case). Everything else (`file`, `gopher`, ...) is out.
2. Reject a URL that smuggles **credentials** with `@` in the authority: in `http://api.cdn-example.test@evil.test/` the real host is `evil.test`. A backslash in the authority is rejected too, since parsers disagree about it.
3. Only the default port for the scheme: 80 for `http`, 443 for `https` (explicit or implied).
4. The host (lowercased, one trailing dot removed) must match the allow-list.
5. The name is resolved **once** and the address must not be internal: `0.0.0.0/8`, `10/8`, `100.64/10`, `127/8`, `169.254/16`, `172.16/12`, `192.168/16`, and everything from `224.0.0.0` up. An allowed name that points inside is the **DNS rebinding** trick. And because an attacker-controlled name can answer differently the second time, the caller must **connect to the address you validated** (`*pinned`), never resolve again.
6. **Redirects** are new requests. A `Location` is checked with the same policy. A path-only redirect such as `/next` stays on the validated host.

When you write the report for an issue like this, the fix is an allow-list and a resolved-address check. See [the finding report lesson](/learn/ethical-hacking/finding-report) for how to say so.
