---
title: Persistence: spot what changed at startup
summary: Compare a baseline of startup, schedule and service entries with a current snapshot and rate each new or altered entry by risk.
minutes: 30
difficulty: medium
tags: persistence, detection, baselining
id: ethical-hacking/persistence
mode: stdout
---
> **Simulated target.** Everything in this lesson is made up and runs inside the sandbox: the snapshots on stdin are invented text listing made-up entries and paths. In scope: the text you read. Nothing is installed, scheduled or started. Never use a technique from this course on a system you do not own or do not have written permission to test. Start with [Law, ethics and permission](/learn/ethical-hacking/law-and-ethics) if you have not yet.

When an attacker gets in, the first worry is *staying* in. A reboot, a logout or a killed process should not end the visit. They arrange for their code to run again by itself, which is called **persistence**. The mechanisms are mundane features of every operating system:

- **startup entries**: programs started at boot or login,
- **scheduled jobs**: cron-style or timer tasks that run at set times,
- **services**: long-running background programs, often as the most privileged user.

None of these is malicious by itself, which is why persistence is hard to see in a list of hundreds of entries. What gives it away is **change**. A defender keeps a **baseline** of how a healthy machine looks and compares the present against it. Anything new or altered is worth a look, and some traits make it worth a *closer* look:

- the program lives in a **temp** location (`/tmp/`, `/var/tmp/`, `/dev/shm/`), which any user can write to and the system never installs software into,
- it lives under a user's **home** directory but runs from a system mechanism,
- the file name is **hidden** (starts with a dot),
- it runs as **root**, so a small foothold turns into a large one.

Input: lines `KIND NAME PATH USER` where KIND is `startup`, `schedule` or `service`. A line `# baseline` begins the baseline section and `# current` the current snapshot. The lesson on [privilege escalation](/learn/ethical-hacking/privesc-misconfig) shows why a root-run entry in a writable place is the worst combination.
