---
title: Read beyond a toy artifact root
summary: Craft a path traversal request and distinguish a protected-file marker from a reflected filename.
minutes: 40
difficulty: hard
tags: security, offensive-lab, artifact-escape
id: sec-infra-offense/artifact-escape
mode: harness
---

All targets in this track are fictional in-process state machines. `Lab.Send` is a callback, not a network client. Only use the port(s) supplied by the exercise. No real programs, accounts, services, credentials or files are touched. Status codes resemble HTTP for readability but describe the toy protocol only.

A download service should confine requests to its artifact root. The vulnerable model checks the raw prefix before normalizing the path. A path beginning with the allowed directory can then use `..` to leave it. The hardened model normalizes first and enforces the boundary.

The target stores a protected synthetic file alongside the download directory. Read its fresh marker to demonstrate the boundary failure. A reflected request path or error message is not proof of file access. Correct confinement must account for normalization and, in real filesystems, symbolic links and race conditions too; those details are deliberately absent from this toy model.
