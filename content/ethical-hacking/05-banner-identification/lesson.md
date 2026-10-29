---
title: Identify services from banners
summary: Parse fake service banners into protocol, product and version, compare versions against a made-up advisory list and handle banners that hide their version.
minutes: 30
difficulty: medium
tags: recon, fingerprinting, vulnerability-management
id: ethical-hacking/banner-identification
mode: harness
---
> **Simulated target.** Everything in this lesson is made up and runs inside the sandbox: every banner and product (quillsh, mailtoy, finchftpd, toyhttpd) is invented, and the advisory list is invented too. In scope: the strings the tests give your functions. No real software or vulnerability is modelled. Never use a technique from this course on a system you do not own or do not have written permission to test. Start with [Law, ethics and permission](/learn/ethical-hacking/law-and-ethics) if you have not yet.

When a [scan](/learn/ethical-hacking/port-scanner) finds an open port, the next question is *what is it?* Many services announce themselves in a **banner**, the first line or header they send. An attacker reads banners to choose what to try. A defender reads the same banners to build an inventory and to see which services are older than the fixed version.

Four invented formats:

```text
SSH-2.0-quillsh_3.2 optional comment
220 mail.example.test ESMTP mailtoy 2.4.1 ready
220 (finchftpd 0.9.8) ready
HTTP/1.1 200 OK<CR><LF>Server: toyhttpd/1.0.3<CR><LF>...
```

**Fingerprinting** has limits. A banner is just text the service chose to send, so it can be wrong or deliberately vague. Hardening guides recommend hiding versions (`220 mail.example.test ESMTP`, `Server: web`). That stops casual identification, but it is **not** a fix: the flaw is still there. What it changes for you as a defender is that your own inventory can no longer tell, so a banner with no version must be reported as `unknown, review by hand` rather than "fine".

## Versions are not strings

`1.10` is newer than `1.9`, and `1.0` equals `1.0.0`. Compare dotted versions component by component as numbers, treating a missing component as 0 and ignoring any non-numeric tail such as `-rc1`.
