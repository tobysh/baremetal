---
title: Role and ownership checks
summary: Write a deny-by-default authorisation function that combines roles with object-level checks on who owns a document and which organisation it belongs to.
minutes: 35
difficulty: medium
tags: security, access-control, authorization, rbac, deny-by-default
id: sec-appsec/role-ownership
mode: harness
---
Logging in proves who someone is. **Authorisation** decides what they may do, and it has to be decided on the server for every request. Broken access control is consistently at or near the top of web vulnerability lists, and it usually comes from two missing ideas.

**Deny by default.** The check starts from "no" and each rule names a case that earns a "yes". The opposite shape (a list of things to forbid) fails the first time someone adds an action, a role or a bug-shaped input that the list did not think of. An unknown role, an unknown action, a missing object or a null user must all come out as denied.

**Object-level checks.** A role says what *kind* of thing someone can do; it does not say to *which* object. A member may edit documents, but only theirs; an editor may edit others' documents, but only inside their own organisation; an admin has the most power, but still stops at the organisation boundary. Leaving out the object check is the bug behind insecure direct object references, as in [the IDOR lesson](/learn/ethical-hacking/idor). Checking only on single objects is not enough either: a list endpoint must filter with the same function, or it leaks what the single-object route hides.

Order matters. The tenant check comes before any role privilege, and a `locked` flag overrides ownership. Keeping one function that every code path calls makes the rules easy to audit and test, and it fits with the gate you will build in [the request-gate review](/learn/sec-appsec/review-request-gate).
