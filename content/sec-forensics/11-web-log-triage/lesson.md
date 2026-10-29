---
title: Triage a web access log
summary: Count status classes in a synthetic access log, set aside malformed lines and flag sources that repeatedly request traversal-style paths.
minutes: 30
difficulty: medium
tags: security, log-analysis, parsing
id: sec-forensics/web-log-triage
mode: stdout
---
A web server writes one access-log line per request: the client address, a timestamp, the quoted request line, the status code and the response size. Logs are evidence, but they are also data an attacker partly controls: the request line and path come straight from the client. A parser must not assume that every line is well formed, and an investigator should count the lines that did not parse rather than dropping them in silence, because a sudden rise in malformed lines is itself a finding.

Here you triage a made-up log. For each line decide whether it is well formed: a first token (the address), a quoted request with exactly three words, then a status between 100 and 599 and a size that is a number or `-`. Count well-formed lines by status class. Then look at the path in each request for the traces of a traversal attempt: `../`, an encoded dot dot (`%2e%2e`, any letter case) or a request for `/etc/passwd`. The match is crude on purpose; a real detection would decode the path first and consider the response (a 404 for a probe is an ordinary outcome).

A source is only flagged when it made two or more such requests, because one odd line proves little. A flag means what the log says; addresses can be shared or forged, and the log does not show whether the probe worked.
