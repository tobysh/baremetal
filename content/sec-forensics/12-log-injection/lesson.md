---
title: Stop log injection
summary: Escape newlines and control bytes before untrusted text enters a log line, so an attacker cannot forge entries. Handle truncation without splitting an escape.
minutes: 30
difficulty: medium
tags: security, log-analysis, input-validation
id: sec-forensics/log-injection
mode: harness
---
Log files are line-oriented, and analysts trust that one line is one event. If an application copies a user-supplied value into a log line unchanged, that user can include a newline followed by a fake line such as a successful login, an administrator action or an entry that looks like a harmless system message. This is log injection, also called log forging. It damages the evidence you need most during an incident, and it can attack the tools that read the logs, such as terminals that interpret control sequences.

The defence is to encode untrusted text on its way into the log: make every byte that could change the structure visible and harmless. Common choices are `\n` and `\r` for line breaks, a doubled backslash so the encoding is unambiguous (otherwise an attacker could write the literal text `\n`), and `\xHH` for other control bytes. Bytes that are printable stay as they are.

This function writes the escaped form into a fixed buffer and, like `snprintf`, returns the length the full result would need. If the buffer is too small, truncate between escapes and never in the middle of one: a half-written `\x0` would be ambiguous and could be misread. The strings are made up.
