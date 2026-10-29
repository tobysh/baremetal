---
title: Data must stay data
summary: Fix a format-string boundary so percent directives in user text are printed literally.
minutes: 25
difficulty: easy
tags: security, defensive-programming
id: sec-c-bugs/format-strings
mode: harness
---
The first argument of `printf` is a miniature program. `%s` fetches a pointer, `%x` fetches an integer, and `%n` writes through a pointer. Calling `printf(user_text)` lets untrusted text choose operations and consume arguments that were never supplied. It is a format-string vulnerability even if the buffer containing the text is perfectly bounded.

Keep the format a literal you control and pass the text as a value: `printf("%s", user_text)`. The same rule applies to `snprintf`; a bounded destination does not make an uncontrolled format safe. Here we build a bounded log message using a literal `"%s"` format. Percent signs, newlines and empty strings stay ordinary bytes.

`snprintf` returns the number of characters that would have been written, excluding the terminator, even if the buffer was too small. That result is useful for detecting truncation. Our wrapper returns this required length as `size_t`, and treats a NULL text as empty. This is only a formatting repair: log injection through newlines is a separate policy question. The exercise deliberately preserves text exactly rather than introducing escaping rules.

## The unsafe pattern

```c
snprintf(dst, cap, user_text);
```

Destination bounds do not make an attacker-controlled format safe. Keep the format literal and supply user text as data.
