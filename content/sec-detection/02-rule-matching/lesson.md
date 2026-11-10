---
title: Evaluate Sigma-style rules
summary: Load simple field rules (equals, contains, starts with, ends with) and report which rules each event triggers.
minutes: 30
difficulty: medium
tags: security, detection, rules
id: sec-detection/rule-matching
mode: stdout
---
A detection rule is data, not code. Community rule formats describe a match as a field, an operator and a value, so analysts can share and review rules without recompiling a sensor. Your engine reads rules first, then streams events through them, and reports which rules each event triggered. That list is what an analyst sees first, so it has to be exact and in a stable order.

## The four operators

Operators are case-sensitive here. `eq` compares the whole value, `contains` looks for a substring, `startswith` and `endswith` compare the ends of the value. Each one is a short function over two strings:

```text
RULE netcat    image endswith /nc
RULE tmp-exec  image startswith /tmp/
RULE enc-shell cmd   contains -enc
RULE root      user  eq root

host=web1 user=deploy image=/usr/bin/nc cmd="backup.sh"
host=db2  user=root   image=/tmp/x/drop cmd="powershell -enc AAAA"
host=db3  user=dave   cmd="ls"
```

```text
event 1: netcat
event 2: tmp-exec,enc-shell,root
event 3: -
```

Event 3 has no `image` field at all, so `netcat` and `tmp-exec` cannot match it, and none of the other rules applies either.

## Pitfalls

- **Missing is not empty.** A rule never matches an event that lacks the field. If you treat a missing field as the empty string, `startswith` with an empty value or `eq` with an empty value fires on events of the wrong kind.
- **Length before compare.** `endswith` needs the field to be at least as long as the value before you compare the last bytes, otherwise you read before the start of the string.
- **Rule order.** The output lists matching rules in the order they were defined, not alphabetically and not by how specific they are.
- **Values with spaces.** The value is the rest of the line, so `RULE r cmd contains nc -l` searches for the three characters `nc -l`.
- **Whole keys only.** Reuse the field extractor from [the previous lesson](/learn/sec-detection/field-extraction) (it is included in your starter), including its first-occurrence rule, so a rule on `user` cannot be fooled by `username`.

## Why defenders care

Good rule engines are boring on purpose; the value is in how clear the rules are. A rule file is reviewed like code, versioned, and replayed against old logs, which only works if the engine is deterministic and a rule that names a field really means that field. Later lessons combine several of these tests with [boolean conditions](/learn/sec-detection/boolean-conditions), count them over [time windows](/learn/sec-detection/threshold-window) and tune them with allowlists.
