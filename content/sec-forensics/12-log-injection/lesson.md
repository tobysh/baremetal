---
title: Stop log injection
summary: Escape newlines and control bytes before untrusted text enters a log line, so an attacker cannot forge entries. Handle truncation without splitting an escape.
minutes: 30
difficulty: medium
tags: security, log-analysis, input-validation
id: sec-forensics/log-injection
mode: harness
---
## The forensic question

An analyst reads `auth.log` and sees a successful administrator login at 02:14. Did it happen, or did a user type those words into a login form, with a newline in the middle, so that the application wrote a second, forged line? If untrusted text reaches a log unchanged, the analyst cannot tell. That attack is **log injection** (log forging).

## What goes wrong

The application logs a failed login with the username the client sent. Suppose the client sends `bob`, a newline, then `INFO login ok user=admin`:

```text
raw log written by a vulnerable application:
2026-03-03T02:14:09Z WARN login failed user=bob
INFO login ok user=admin

what the analyst thinks happened: two events
what really happened: one event, one forged line
```

A control byte can also attack the reader: terminal escape sequences can rewrite what is shown on screen.

## The defence: encode on the way in

Make every byte that could change the log's structure visible and harmless:

```text
input byte            written as
0x0a  newline         \n          (two characters)
0x0d  carriage return \r
0x5c  backslash       \\          (so an attacker's literal \n stays distinguishable)
0x00-0x1f, 0x7f other \xHH        (four characters, lowercase hex)
everything else       copied unchanged, including bytes 0x80 and above
```

With that rule the forged input becomes one line: `bob\nINFO login ok user=admin`, and a reader (or a decoder) can still recover the original exactly.

## Worked example

Escape `a<TAB>\<LF>` (a, a tab, a backslash, a newline) into a buffer of capacity 6.

1. `a` is plain: piece `a` (1 character), total 1.
2. Tab is 0x09, a control byte other than LF/CR: piece `\x09` (4 characters), total 5.
3. Backslash becomes `\\` (2 characters), total 7.
4. Newline becomes `\n`, total 9.

The complete result `a\x09\\\n` needs 9 characters, so the function returns 9. With capacity 6 there is room for 5 characters plus the terminator: `a` fits (w=1) and `\x09` fits (w=5); the backslash piece would need 7, so writing stops and the buffer holds `a\x09`. Nothing is written after that, even though a one-character piece could still fit: stopping at the first piece that does not fit keeps the output a clean prefix of the full result, and it never cuts an escape in half. A half-written `\x0` would be ambiguous.

## Pitfalls

- **Return the full length**, like `snprintf`, so the caller can detect truncation.
- **Escape the backslash itself**, otherwise escaped and literal text collide.
- **Tab is a control byte**: `\x09`, not `\t`.
- **Unsigned bytes.** Use `unsigned char` so 0x80 and above are not mistaken for negative values below 0x20.
- **Terminate** whenever `cap > 0`; with `cap == 0` or a NULL buffer write nothing.

## What a defender concludes

Escaping at write time keeps one line equal to one event. It does not make old logs trustworthy, and it does not replace restricting who can write to them; combine it with structured logging and shipping logs off the host.
