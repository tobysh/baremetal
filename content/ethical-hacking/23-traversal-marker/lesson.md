---
title: Prove path traversal with a marker
summary: Try approved file probes and confirm a boundary crossing using an exact synthetic marker instead of a status code.
minutes: 35
difficulty: medium
tags: offsec, web, assessment
id: ethical-hacking/traversal-marker
mode: harness
---
> **Simulated target.** Only the in-memory callback supplied by this lesson is in scope. All accounts, files and markers are fictional. No network requests or host file access are needed. Start with [Law, ethics and permission](/learn/ethical-hacking/law-and-ethics).

A download endpoint may join a requested name to a base directory without keeping the resolved path inside that directory. Parent segments such as `../` can then cross the intended boundary. Encoding and decoding order can change which inputs reach that operation.

Your fictional lab has an ordinary control file and a harmless marker outside its allowed download directory. The target callback simulates fetching a candidate; it never opens a host file. Approved candidates are supplied as data, including plain and encoded forms. Send them unchanged: decoding in your assessment code would test a different input from the one you recorded.

First fetch the normal control and require its exact expected body. Then try approved candidates in order. A candidate confirms traversal only when it returns status 200 and the **entire** marker. A generic error mentioning the marker, or a page with the same length, is insufficient. Stop at the first proof. This keeps evidence minimal and makes the finding reproducible.

A negative result covers only this candidate list. It does not establish that every encoding, separator, operating system or symlink is safe. The exercise deliberately models strings rather than a complete filesystem.

For remediation, prefer mapping opaque download IDs to approved files. Where paths are necessary, enforce containment during file resolution and account for symlinks and races; a string prefix check or a blacklist of `../` is insufficient. Retest the normal download and the successful marker probe after the fix.

Further reading: [OWASP directory traversal testing](https://wstg.owasp.org/stable/4-Web_Application_Security_Testing/05-Authorization_Testing/01-Testing_Directory_Traversal_File_Include/).
