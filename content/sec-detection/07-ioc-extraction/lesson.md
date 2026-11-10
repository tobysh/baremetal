---
title: Extract indicators from a report
summary: Pull addresses, domains and hashes out of free text, refang them, deduplicate and sort, while ignoring look-alikes.
minutes: 40
difficulty: hard
tags: security, detection, ioc
id: sec-detection/ioc-extraction
mode: stdout
---
Threat reports and incident tickets are written for people, but defenders need their indicators of compromise (IOCs) in a form a firewall or SIEM can load. Extraction by hand is slow; extraction by a careless regular expression is wrong. A pattern like `\d+\.\d+\.\d+\.\d+` accepts `300.1.1.1`, `007.1.1.1` and the first four numbers of `10.2.3.4.5`, which fills block lists with garbage, and a block list full of garbage either blocks the wrong thing or erodes trust in the list.

## Refang, tokenise, classify

Authors also defang indicators so that nobody clicks them by accident: `198[.]51[.]100[.]7` or `evil(.)test`. The pipeline has three steps:

1. **Refang** the line by turning `[.]` and `(.)` back into `.`.
2. **Tokenise** into runs of letters, digits, `.` and `-`. Anything else (spaces, `:`, `/`, `?`, `,`, brackets) is a separator. Trim dots from both ends of each token, because a sentence may end in `.`.
3. **Classify** each token by strict rules, lowercase first.

The strict rules: an IPv4 address has exactly four decimal parts of 0 to 255 with no leading zeros (`007` fails, `0` passes); a domain here ends in `.test` and has at least two non-empty labels made of letters, digits and hyphens that neither start nor end with a hyphen; a hash is exactly 32 or 64 hexadecimal digits. Domains and hashes are lowercased. Everything is fictional: addresses come from documentation ranges and domains end in `.test`.

```text
Beacon to 203(.)0(.)113(.)36 from 198.51.100.7, then GET http://Login.Pay-Secure.TEST/a?x=1.
Ignore 300.1.1.1, 10.2.3.4.5 and -bad.example.test.
Digest: D41D8CD98F00B204E9800998ECF8427E
```

```text
ip 198.51.100.7
ip 203.0.113.36
domain login.pay-secure.test
hash d41d8cd98f00b204e9800998ecf8427e
```

The output is sorted by type, then by `strcmp` on the printed value, so `198.51.100.200` sorts before `198.51.100.7`. Each indicator appears once however often it is mentioned.

## Pitfalls

- **Leading zeros and range.** Check digit counts and the 0 to 255 range per part; do not trust `atoi`.
- **Too many parts.** `10.2.3.4.5` is neither an IP nor a domain and is dropped whole, not trimmed to four parts.
- **Hash length.** 31 or 33 hex digits is not a hash; a 32-digit run is a hash even if it is all digits.
- **Case.** `EVIL.TEST` and `evil.test` are the same indicator; lowercase before deduplicating.
- **Trailing punctuation.** `evil.test.` at the end of a sentence is `evil.test`, but a dot in the middle is part of the name.

## Why defenders care

Strict validation and normalisation are what make an IOC list safe to feed to automated blocking, a theme you will see again in [log injection](/learn/sec-forensics/log-injection): extraction is a trust boundary, and what crosses it must be well formed.
