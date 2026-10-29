---
title: Broken access control and IDOR
summary: A toy invoice handler returns any record whose number you guess. Fix the authorization check, hide which ids exist, and spot the guessing in an access log.
minutes: 30
difficulty: medium
tags: web, access-control, idor, detection
id: ethical-hacking/idor
mode: harness
---
> **Simulated target.** Everything in this lesson is made up and runs inside the sandbox: the shop, its invoices and its users are test data inside the harness. In scope: the handler function you fix and the access log you analyse. No real application is probed. Never use a technique from this course on a system you do not own or do not have written permission to test. Start with [Law, ethics and permission](/learn/ethical-hacking/law-and-ethics) if you have not yet.

An **insecure direct object reference (IDOR)**, a form of **broken access control**, is one of the most common and most damaging web flaws, and one of the simplest. The application looks up a record by the number in the request, `GET /invoice/1042`, and returns it without asking whether *this user* may see it. The user is logged in, so authentication passed. Nobody checked **authorization**. An attacker who is allowed to see invoice 1042 just changes the number to 1043, 1044...

The fix is a check on **every** access, on the server, using the identity the server already knows (not a field the client sends): the caller must **own** the record or have the admin role. Two details separate a good fix from a half fix:

- **Do not reveal what exists.** If a forbidden id returns "403 forbidden" and a missing id returns "404", an attacker can map which ids are real. Return the same `404` for "not yours" and "does not exist".
- **Unguessable ids** help (random identifiers instead of 1, 2, 3) but are only a speed bump. The check is the fix.

Your second job is detection. Someone walking through ids leaves a trail of `404`s from one account in a short time. The access log shows it even when the attack fails, and a defender who sees it can lock the account and review it. The same idea as [detecting a scan](/learn/ethical-hacking/detect-the-scan), applied to an application.
