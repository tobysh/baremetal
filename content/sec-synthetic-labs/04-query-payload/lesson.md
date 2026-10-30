---
title: Phase 2: break a toy query builder
summary: Craft a payload that returns unauthorized rows, and explain why binding parameters blocks it.
minutes: 35
difficulty: medium
tags: security, synthetic-lab, query-payload
id: sec-synthetic-labs/query-payload
mode: harness
---

The fictional lookup concatenates a name into `SELECT id FROM users WHERE name='<input>'`. The toy evaluator recognizes one injection grammar: a closing quote, ` OR 1=1 --` and an optional suffix. It treats that condition as true for every row. This deliberately small grammar teaches a data-to-code boundary; it is not a SQL engine.

Your job is to provide attacker-controlled input. Hidden tests create different users and prove that the input returns a row that does not belong to the requested name. They also send the same payload through a bound-value lookup: there it must return no unauthorized rows. Detection by searching for quotes is not a substitute for binding values.

Use Run to check compilation. Test supplies varied hidden inputs. All data and targets are fictional; nothing connects to an external service.
