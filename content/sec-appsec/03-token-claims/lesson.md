---
title: Validate token claims and the clock
summary: After the signature checks out, enforce expiry, not-before, audience and issuer with a small clock-skew allowance.
minutes: 45
difficulty: hard
tags: security, auth, tokens
id: sec-appsec/token-claims
mode: stdout
---
A valid signature proves who made a token, not that it is acceptable now, here and for this purpose. The payload carries claims that you must check yourself. `exp` is the first moment the token is no longer valid, `nbf` the first moment it is, `aud` says which service the token is for and `iss` which issuer made it. Skipping `aud` is the classic error: a token minted for one service gets replayed against another that shares the same key.

Servers' clocks disagree by a few seconds, so verifiers allow a small skew. With skew `s`: the token is expired when `now > exp + s`, and not yet valid when `now + s < nbf`. Boundaries matter, because off-by-one mistakes here either lock users out or keep dead tokens alive: `now == exp + s` is still acceptable.

Be strict in the parsing, too. A missing `exp` means a token that never expires, which you should refuse. Numbers must be plain digits (no sign, no letters, at most 18 digits so they fit in 64 bits), and duplicated claim keys make the payload malformed, exactly as for the header in [token signatures](/learn/sec-appsec/token-signature). The decoder and MAC are provided again in the starter.
