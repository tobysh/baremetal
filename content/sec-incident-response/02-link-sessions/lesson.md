---
title: Link web sessions to accounts
summary: Attribute web requests to accounts from session and address evidence, and report how strong each link is, including ambiguous and unknown cases.
minutes: 35
difficulty: medium
tags: incident-response, correlation
id: sec-incident-response/link-sessions
mode: harness
---

A web log says which address and session made a request. An authentication log says which account logged in from which address. Neither names the person behind a request, so the link between them is something you derive, and it has different strengths.

- A **session identifier** that the login service issued to an account is strong evidence for that account, even if the address changed (which may itself be worth noting).
- An **address plus time** is weaker. If exactly one account logged in from that address shortly before, it is a probable match. If several did (a shared office or carrier NAT), the honest answer is "ambiguous", not a guess.
- A session identifier that no login ever issued is **unknown**. Falling back to the address would be wrong: the address might belong to the intruder, and you would blame whoever else used it.

Reporting the basis next to every link lets a reader see which conclusions are observed and which are inferred.

**Limitation:** all data here is invented and tidy. Real logs have missing fields, shared accounts and addresses that change mid-session.
