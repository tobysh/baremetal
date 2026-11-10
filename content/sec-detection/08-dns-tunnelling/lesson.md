---
title: Detect DNS tunnelling
summary: Flag domains whose query names look like smuggled data: many unique, long subdomains, often TXT or NULL records.
minutes: 40
difficulty: hard
tags: security, detection, dns, network
id: sec-detection/dns-tunnelling
mode: stdout
---
DNS is almost always allowed out of a network, which makes it a favourite channel for hiding data: the sender encodes bytes into subdomain labels (`mfrgg43jnzsxi5dp.t1.test`) and the resolver path carries them to a server that controls the zone. After [reading DNS in the packet course](/learn/sec-packets/dns) you can see why this works and what it looks like. The defender cannot block DNS, so the defender has to notice its shape.

## What a tunnel looks like

Normal traffic has few distinct names that repeat (`www`, `mail`, `api`). Tunnels have a high count of distinct subdomains, each long, rarely repeated, and often asked as TXT or NULL records, which carry more payload. Neither signal is enough alone. A CDN has many distinct subdomains too, but they are short (`img12`); a rarely used service may have long names but only a handful. Combine them, and write down the threshold you used, since a number you cannot explain is a number you cannot tune.

For each registered domain (the last two labels, case-insensitively), the subdomain is everything before it. Count total queries Q, the distinct non-empty subdomains (call the count U) and the total length S of those distinct subdomains in characters, dots included. A domain is suspect when U is at least 8 and the average length S/U is at least 20, which in integers means `S >= 20 * U`.

## Worked example

```text
10 192.0.2.5 www.corp.test A
11 192.0.2.5 www.corp.test A
12 192.0.2.5 img1.cdn.test A
13 192.0.2.5 img2.cdn.test A
14 192.0.2.9 mfrgg43jnzsxi5dpmfrgg43jnzsxi5.t1.test TXT
15 192.0.2.9 nbswy3dpeb3w64tmmqqhg5dsnfxgo.t1.test TXT
```

```text
corp.test: Q=2 U=1 S=3   -> not suspect (few, short)
cdn.test : Q=2 U=2 S=8   -> not suspect (short)
t1.test  : Q=2 U=2 S=59  -> U below 8, so not suspect yet
```

Add six more long (about 30 characters), distinct `t1.test` names and the domain reaches U=8 with S above 160, and the report line starts `t1.test unique=8 avg=29` (the average is cut down by integer division).

## Pitfalls

- **Distinct means distinct.** Ask `aaaa.t1.test` a thousand times and U is still 1. Query counts alone are not the signal; repeats are normal.
- **Case.** Names are case-insensitive, so `ABC.T1.TEST` and `abc.t1.test` are one subdomain of one domain.
- **Dots count.** A subdomain such as `a.b` has length 3, and a name with several labels before the domain is still a single subdomain string.
- **Integer arithmetic.** Compare `S >= 20 * U` rather than dividing; the printed `avg` uses integer division, so 20.9 prints as 20.
- **Bare domain.** A query for just `t1.test` counts toward Q (and txt, if it is TXT) but contributes no subdomain.
- **Ordering.** Print suspects by unique count descending, then by domain name ascending.

## Why defenders care

Thresholds like these are a first filter, not a verdict. Pair the hit with the client list and the record types, and expect to tune the numbers against your own resolver logs. Writing the rule with explicit numbers makes false positives something you can reason about instead of argue about.
