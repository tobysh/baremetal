---
title: Reject replayed messages
summary: Accept a signed message once: check its timestamp window, remember nonces only while they matter and fail closed when the cache is full.
minutes: 45
difficulty: hard
tags: security, auth, replay
id: sec-appsec/replay-window
mode: stdout
---
A signature proves a message is genuine, but not that it is new. An attacker who records a valid "transfer 10 units" message can send it again tomorrow. Replay defence needs two ingredients: a timestamp that bounds how old (or how far in the future) a message may be, and a nonce, a value that must never repeat, remembered by the receiver.

Say the receiver accepts a message when its timestamp is within `W` seconds of the receiver's own clock. Anything older than `now - W` is `stale`; anything later than `now + W` is `future`; a clock-based check without both sides lets an attacker pre-record messages for later. A nonce seen inside the window is a `replay`. Because a message outside the window is rejected anyway, a nonce older than the window can be forgotten, which keeps memory bounded: that is the whole trick. Evict entries whose message time is below `now - W` before deciding.

Memory must have a limit, and the limit must fail safe. If the cache is full of live nonces, refuse new messages (`cache-full`) rather than evict a live nonce, because dropping one would re-enable replay of that message. Rate limiting upstream, as in [rate limiting](/learn/ethical-hacking/rate-limiting) from the ethical hacking track, keeps an attacker from filling the cache cheaply. This lesson uses plain integer timestamps.
