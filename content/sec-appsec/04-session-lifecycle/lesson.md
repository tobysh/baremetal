---
title: Enforce session lifetimes
summary: Track server-side sessions: reject reused or fixated ids, expire them after idle and absolute timeouts, and retire them on logout.
minutes: 40
difficulty: medium
tags: security, auth, sessions
id: sec-appsec/session-lifecycle
mode: stdout
---
A session is the server's memory that a browser logged in. Its identifier is a bearer credential: whoever presents it is treated as the user, so its lifetime and its handling are security decisions.

Two timeouts bound the risk of a stolen identifier. The idle timeout ends a session that has not been used for a while (a stolen cookie from last week should not work). The absolute timeout ends it a fixed time after creation no matter how active it is, so a hijacked session cannot be kept alive forever by clever pings. Here a request at time `t` finds a session expired when `t - created > absolute` (checked first) or `t - last > idle`; an expired session is destroyed on the spot.

Session fixation is the other half. If login accepts an identifier the client already chose, an attacker can plant a known identifier in a victim's browser, wait for the victim to log in, and then use the same identifier. The defence is that login always issues a fresh identifier and never accepts one that is active or was already used and retired. In this simulation the server hands out the identifier on each `login` event, so a login that presents an identifier that exists or ever existed is refused. Compare the weekly counting in [authentication log analysis](/learn/sec-forensics/auth-log-analysis): here the state machine is what matters.
