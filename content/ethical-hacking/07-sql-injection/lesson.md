---
title: Injection: find it, fix it, log it
summary: A toy database accepts SQL text. See how a quote in a name rewrites the query, fix the lookup with a parameter, and add a detector for the log.
minutes: 30
difficulty: medium
tags: web, injection, secure-coding
id: ethical-hacking/sql-injection
mode: harness
---
> **Simulated target.** Everything in this lesson is made up and runs inside the sandbox: the 'database' is a few lines of test code that understands one made-up query shape. In scope: the lookup function you edit. No real database or web application is involved. Never use a technique from this course on a system you do not own or do not have written permission to test. Start with [Law, ethics and permission](/learn/ethical-hacking/law-and-ethics) if you have not yet.

**Injection** happens when untrusted input is pasted into a command in a way that lets the input change what the command *means*. SQL injection is the classic case. Here is the vulnerable shape:

```c
snprintf(sql, sizeof sql, "SELECT id FROM users WHERE name='%s'", name);
db->exec_text(db, sql, ids, cap);
```

For the name `alice` the query asks for rows named alice. For the name `x' OR '1'='1` the quote ends the string early, and the rest of the input becomes part of the query: `... WHERE name='x' OR '1'='1'`. Now the condition is always true and every row comes back. An attacker uses the same trick to skip a password check, read other tables or comment out the rest of a query with `--`.

The attacker's view of this takes seconds to try. The defender's fix is just as short, and it is the same fix as always: **keep code and data separate**. A **parameterized query** sends the query template (`... WHERE name = ?`) and the value as two separate things. Whatever is in the value, the database treats it as a value, never as syntax. Escaping quotes by hand is the fragile alternative and is not the answer.

## Three jobs

1. Fix `find_user_ids` so it only uses `exec_param`. A name like `O'Brien` is a perfectly good name and must still find its owner.
2. A second line of defence is **visibility**. `looks_like_injection` should flag input for the security log. It is a tripwire for the log, not a filter: never *rely* on it to block.
3. Notice how both functions treat the same hostile text differently.

The toy database below the harness only understands `SELECT id FROM users WHERE name='...'`. Its text interface can be fooled; its parameter interface only accepts the template `SELECT id FROM users WHERE name = ?`. Compare this with [the earlier C bugs](/learn/sec-c-bugs/format-strings): the root cause is the same, data treated as instructions.
