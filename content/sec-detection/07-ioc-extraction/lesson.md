---
title: Extract indicators from a report
summary: Pull addresses, domains and hashes out of free text, refang them, deduplicate and sort, while ignoring look-alikes.
minutes: 40
difficulty: hard
tags: security, detection, ioc
id: sec-detection/ioc-extraction
mode: stdout
---
Threat reports and incident tickets are written for people, but defenders need their indicators of compromise (IOCs) in a form a firewall or SIEM can load. Extraction by hand is slow; extraction by a careless regular expression is wrong. A pattern like `\d+\.\d+\.\d+\.\d+` accepts `300.1.1.1`, `007.1.1.1` and the first four numbers of `10.2.3.4.5`, which fills block lists with garbage.

Authors also defang indicators so that nobody clicks them by accident: `198[.]51[.]100[.]7` or `evil(.)test`. First refang the line by turning `[.]` and `(.)` back into `.`, then split it into tokens made of letters, digits, `.` and `-`, and trim any dots from the ends (a sentence may end in `.`). Classify each token by strict rules: an IPv4 address has exactly four decimal parts of 0 to 255 with no leading zeros; a domain here ends in `.test`, has non-empty labels made of letters, digits and hyphens that neither start nor end with a hyphen; a hash is exactly 32 or 64 hexadecimal digits. Lowercase domains and hashes. Everything is fictional: addresses come from documentation ranges and domains end in `.test`.

Strict validation and normalisation are what make an IOC list safe to feed to automated blocking, a theme you will see again in [log injection](/learn/sec-forensics/log-injection).
