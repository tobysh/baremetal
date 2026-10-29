---
title: Detect DNS tunnelling
summary: Flag domains whose query names look like smuggled data: many unique, long subdomains, often TXT or NULL records.
minutes: 40
difficulty: hard
tags: security, detection, dns, network
id: sec-detection/dns-tunnelling
mode: stdout
---
DNS is almost always allowed out of a network, which makes it a favourite channel for hiding data: the sender encodes bytes into subdomain labels (`mfrgg43jnzsxi5dp.t1.test`) and the resolver path carries them to a server that controls the zone. After [reading DNS in the packet course](/learn/sec-packets/dns) you can see why this works and what it looks like.

Normal traffic has few distinct names that repeat (`www`, `mail`). Tunnels have a high count of distinct subdomains, each long, rarely repeated, and often asked as TXT or NULL records, which carry more payload. Neither signal is enough alone. A CDN has many distinct subdomains too, but they are short (`img12`); a rarely used service may have long names but only a handful. Combine them, and write down the threshold you used, since a number you cannot explain is a number you cannot tune.

For each registered domain (the last two labels, case-insensitively), the subdomain is everything before it. Count total queries, the distinct non-empty subdomains (call the count U) and the total length S of those distinct subdomains in characters, dots included. A domain is suspect when U is at least 8 and the average length S/U is at least 20, which in integers means `S >= 20 * U`.
