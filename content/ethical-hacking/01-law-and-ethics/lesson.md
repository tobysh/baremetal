---
title: Law, ethics and permission
summary: The mandatory first lesson: authorization, written scope, rules of engagement, computer-misuse law in general terms and coordinated disclosure, with a gate.
minutes: 25
difficulty: easy
tags: ethics, law, scope, disclosure
id: ethical-hacking/law-and-ethics
mode: harness
---
> **Simulated target.** This lesson is about rules, not techniques, and its exercise only checks your reasoning with made-up engagement paperwork and host names ending in `.test`. In scope: your own answers. Never use any technique from this course on a system you do not own or do not have written permission to test. **Complete this lesson before every other lesson in the course.**

Security work is the one craft where the same keystrokes are either a career or a crime, and the only difference is **permission**. This lesson is the gate to the rest of the course. Read it properly, then pass the gate.

## Authorization comes first

You may test a system only when the **owner** (or someone with the authority to speak for the owner) has agreed, **in writing**, before you start. Remember the sentence: **no written permission means do not test.** A verbal "sure, go ahead", a colleague who thinks it is fine, or a system that merely looks abandoned is not permission. Neither is a good intention. Nor is the fact that a flaw is easy to reach.

Permission has to come from the person who actually controls the system. A manager who owns the marketing site cannot authorize you to test the payment provider that site happens to call. Hosted services, cloud platforms and third parties usually have their own rules, and you need their agreement as well.

## Scope: the list is also a fence

The written agreement lists what is **in scope**: hosts, address ranges, applications, the dates and hours of the **test window**, and sometimes the people who may be contacted. Everything else is out of scope, even if it sits next to a target on the same network. These are the **rules of engagement**:

- which targets (for example `web.lab.test` or the wildcard `*.lab.test`),
- which days you may test,
- which techniques are forbidden (denial of service, social engineering of staff, touching real customer data),
- who to call, and what to do the moment something unexpected happens.

If during a test you find a host that is not on the list, you **stop and ask**. You do not "just check" it. If you find sensitive data, you stop, record the minimum evidence, and tell the contact. You do not collect more to prove a point.

## The law, in general terms

Almost every country has computer-misuse laws. The details differ, so this is a general picture and not legal advice. Typically it is an offence to access a computer system or data **without authorization**, or to **exceed** the authorization you were given, and sometimes to damage, disrupt or intercept. Notice what is missing from the usual tests: the law rarely asks whether you meant well, whether you found a "real" flaw, or whether you told the owner afterwards. Good intent is not a defence in most places; permission is. When in doubt, ask a lawyer before you act, not after.

## Coordinated disclosure

Sometimes you stumble on a flaw without any engagement at all, for instance while using a service normally. Do not dig further to "confirm" it, because confirming means accessing things you were never allowed to. Stop, keep a note of what you saw, and report it **privately** to the owner (many publish a contact or security policy), or through a coordinator. Give them reasonable time to fix it, and agree on when details become public. This is **coordinated disclosure**. Publishing first, selling it, or showing off to friends harms the people whose data is at risk.

## The gate

Pass both parts below. The tests use many different engagements, so a check that only works for one example will fail.

1. A short true/false quiz. `quiz_answer(n)` returns `1` for true and `0` for false for statement number `n`.
2. `may_proceed` decides, for a planned action, whether to go ahead or stop and why. It is the code version of the paperwork rules.
