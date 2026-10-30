---
title: Exploit a fictional outdated job agent
summary: Cross a toy command boundary and prove execution through an emitted marker, with a patched negative control.
minutes: 40
difficulty: hard
tags: security, offensive-lab, outdated-agent
id: sec-infra-offense/outdated-agent
mode: harness
---

All targets in this track are fictional in-process state machines. `Lab.Send` is a callback, not a network client. Only use the port(s) supplied by the exercise. No real programs, accounts, services, credentials or files are touched. Status codes resemble HTTP for readability but describe the toy protocol only.

Old software is a starting hypothesis, not proof. The fictional `labagent/0.9` concatenates an untrusted job label into a command interpreted by a tiny lab-only language. The patched `labagent/1.0` treats labels as data. These are invented versions and do not describe any real vendor vulnerability.

The toy interpreter recognizes `backup <label>` and `emit <text>`, separated by semicolons. `emit` records a marker in target memory. No shell, process, host command or network connection is executed. The vulnerable boundary is the same data-to-instruction mistake seen in command injection. A banner can lie, so verify the marker through a separate read.
