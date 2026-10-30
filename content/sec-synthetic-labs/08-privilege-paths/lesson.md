---
title: Find root execution paths in synthetic Linux state
summary: Correlate writable files, cron entries and sudo rules to identify privilege escalation candidates.
minutes: 35
difficulty: medium
tags: security, synthetic-lab, privilege-paths
id: sec-synthetic-labs/privilege-paths
mode: harness
---

A privilege path exists when a lower-privilege identity can influence an operation executed with more authority. Finding a writable file alone is weaker evidence than finding a writable script scheduled by root. Correlate the execution configuration with permissions.

The fictional snapshot has a user, supplementary groups, file modes (JSON integers, not octal strings), cron entries and sudo rules. A cron entry is relevant only when RunAs is root. A sudo rule is relevant only when its User is the snapshot user, RunAs is root and NoPassword is true. This lab deliberately models script replacement through a writable file only; directory replacement, ACLs, sudo argument restrictions and program-specific escape behavior are outside its model.

Apply Unix permission class selection: if the user owns a file, only owner bits apply. Otherwise, a matching group uses group bits; only a nonowner outside the group uses other bits. Do not OR all three classes together.

Use Run to check compilation. Test supplies varied hidden inputs. All data and targets are fictional; nothing connects to an external service.
