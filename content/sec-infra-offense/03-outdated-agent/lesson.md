---
title: Exploit a fictional outdated job agent
summary: Cross a toy command boundary and prove execution through an emitted marker, with a patched negative control.
minutes: 40
difficulty: hard
tags: security, offensive-lab, outdated-agent
id: sec-infra-offense/outdated-agent
mode: harness
---

> **Simulated target.** Everything in this lesson is made up and runs inside the sandbox: the only 'host' is a `Lab` object the tests pass to your code, and `Lab.Send` is a plain Go callback, not a socket. In scope: the one port the exercise hands you. The "agent", its "version" and its command language are invented for teaching; no real product, vulnerability or shell is involved. Never use a technique from this course on a system you do not own or do not have written permission to test. Start with [Law, ethics and permission](/learn/ethical-hacking/law-and-ethics) if you have not yet.

Old software is a **hypothesis**, not a result. `labagent/0.9` is an invented job runner with a classic flaw: it pastes your job label into a command string and then interprets the whole string. `labagent/1.0` treats the label as plain data. Your job is to test the hypothesis and prove the outcome, not to trust the version text.

## The model

The toy interpreter works on one string, split on semicolons. It knows two commands, `backup <anything>` (no effect) and `emit <text>` (stores `<text>` as the last proof in the target's memory). Nothing is executed on any host.

```text
label you send:      nightly; emit LAB-PROOF:x
agent builds:        "backup " + label
                  =  "backup nightly; emit LAB-PROOF:x"
split on ';':        [ "backup nightly" ]  [ " emit LAB-PROOF:x" ]
interpreted:         backup nightly (no effect)    emit LAB-PROOF:x  -> last proof = LAB-PROOF:x
```

The flaw is data crossing into code: the label was supposed to be a name, but the `;` made part of it a command. This is the same mistake as command injection.

## A worked example

```text
GET  /version      -> 200 "labagent/0.9"                  precondition holds
POST /job {"label":"nightly; emit LAB-PROOF:execution-2"} -> 202   accepted (says nothing yet)
GET  /last-proof   -> 200 "LAB-PROOF:execution-2"         the marker appeared: execution proven
```

Against a patched target the same three calls give `202` for the job and an empty `/last-proof`. The `202` was never evidence. Build the JSON with `json.Marshal`:

```go
body, _ := json.Marshal(map[string]string{"label": "nightly; " + "emit " + marker})
```

## Pitfalls

- Attacking without reading `/version` first, or attacking a `1.0` banner. The precondition is part of the exercise.
- Believing the job reply. Even a patched agent says `202`; only an independent read of `/last-proof` shows effect.
- Comparing loosely. The proof must equal the supplied marker exactly, not merely contain it.
- Hardcoding a marker or sending more than one job. Each scenario supplies a new marker.

## The defender's half

The repair for this class of bug is to keep data and code apart: pass the label as a parameter, never splice it into an interpreted string, and validate it against an allow-list such as letters, digits and dashes. Updating the agent helps, but patch management also needs to verify the effect, because a banner can lag behind (or ahead of) the real software.
