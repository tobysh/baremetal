---
title: Scope how far a stolen credential spread
summary: Propagate compromise through logins and secret reads in time order, ignoring events that happened before the actor was compromised.
minutes: 40
difficulty: hard
tags: incident-response, scoping, lateral-movement
id: sec-incident-response/scope-spread
mode: harness
---

Once you know one credential was stolen, the next question is what else it exposed. Compromise spreads along events: a compromised credential that logs in to a host compromises that host, and every credential stored on that host; a compromised credential that reads a secret compromises that secret.

**Time matters.** The same credential may have made perfectly legitimate logins before it was stolen. Those events prove nothing about spread. Only events at or after the moment an actor became compromised count. Going through events once in time order is tempting and breaks when several events share a second: the order inside that second is arbitrary in the logs, so treat events in the same second as simultaneous and repeat until nothing new is found.

The answer is a scope list with the first moment each item is known to be exposed. It is an upper bound under the model's rules, not a measurement of what the intruder actually touched.

**Limitation:** the model assumes every login by a compromised credential is the intruder's, and that a read of a secret exposes it. Real scoping needs evidence about what was actually done.
