---
title: Capstone: chain a leak into management access
summary: Combine scoped discovery, leaked configuration and a service token to prove unauthorized management access.
minutes: 40
difficulty: hard
tags: security, offensive-lab, chained-compromise
id: sec-infra-offense/chained-compromise
mode: harness
---

All targets in this track are fictional in-process state machines. `Lab.Send` is a callback, not a network client. Only use the port(s) supplied by the exercise. No real programs, accounts, services, credentials or files are touched. Status codes resemble HTTP for readability but describe the toy protocol only.

An infrastructure compromise may require several individually modest findings. A public backup config reveals a management port and a storage key; an exposed value reveals a token; the token grants access to a privileged snapshot. The impact comes from the entire chain.

Keep scope throughout the chain. A server-provided port is untrusted evidence, not permission to contact it. The management port must still be in the caller's supplied scope. Do not turn a leaked token into a reported success until a protected operation returns fresh evidence. Remediation requires closing the leak and rotating the exposed secret; fixing only one link may leave existing credentials usable.


## The model

```text
candidate ports (your scope)  ──GET /banner──▶  labbackup/0.8 ?  no -> next candidate
backup port  ──GET /public/config──▶ {management_port, token_key}
                  management_port in your scope?  no -> next candidate (never contact it)
backup port  ──GET /public/value/<token_key>──▶ {token}
management port ──GET /admin/snapshot (Token: leaked)──▶ 200 "LAB-PROOF:..."   <- the finding
```

Each step is modest: a banner, a public config, a readable value. The impact is the whole chain, and every link is a gate that can stop you.

## A worked example

Scope `[9102, 9100, 9300]`; the backup service is on 9100, the management service on 9300:

```text
9102 GET /banner -> "labother/1.0"                 skip
9100 GET /banner -> "labbackup/0.8"                continue
9100 GET /public/config -> {"management_port":9300,"token_key":"management-key-2"}   9300 is in scope
9100 GET /public/value/management-key-2 -> {"token":"leaked-token-2"}
9300 GET /admin/snapshot  Token leaked-token-2 -> 200 "LAB-PROOF:snapshot-2"
return Finding{Port: 9300, Evidence: "LAB-PROOF:snapshot-2"}
```

If the config had named port `65000`, the right move is to continue without sending anything there: a port a server tells you about is a claim, not permission. Note that the finding reports the **management** port, because that is where the access was proved.

## Pitfalls

- Following a leaked port without checking your scope.
- Sending the token on the public reads. Only the snapshot request carries it.
- Treating `200` as the finish. A decoy can answer `200 login required`; require the `LAB-PROOF:` prefix.
- Not handling bad JSON, an empty key or an empty token, which should end that candidate and not crash it.
- Giving up on the first candidate that fails. Move on to the next port in order.

## The defender's half

Fixing one link leaves the others. Stop the config from publishing the management address and key, protect the stored value, and rotate the exposed token, since the old one still works after the leak is closed. Restrict the management port to a management network so a leaked secret alone is not enough. The challenge below practices that repair: rotation, revocation of the old credential and removal of the leaked value, each confirmed independently.
