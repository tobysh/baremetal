---
title: Write a finding report
summary: Render a structured finding with severity, root cause, evidence and remediation, then write a checker that grades a report against the structure.
minutes: 40
difficulty: medium
tags: reporting, communication, findings
id: ethical-hacking/finding-report
mode: harness
---
> **Simulated target.** Everything in this lesson is made up and runs inside the sandbox: the findings are made-up records and the reports are plain strings checked by the tests. In scope: the text you produce. No system is touched. Never use a technique from this course on a system you do not own or do not have written permission to test. Start with [Law, ethics and permission](/learn/ethical-hacking/law-and-ethics) if you have not yet.

A finding nobody understands is a finding nobody fixes. The report is the product of an assessment. A good one lets a busy engineer, who was not there, see **what is wrong, how bad it is, why it happens, how you know, and what to do**. Five parts, always in this order:

```text
FINDING: <one-line title>
SEVERITY: <Critical|High|Medium|Low|Info>
ROOT CAUSE: <why it happens, one sentence>
EVIDENCE:
- <an observation someone can verify>
- <another>
REMEDIATION: <the specific change that fixes it>
```

Good practice, and what the checks look for:

- **Root cause, not symptom.** "IDOR on the invoice page" is a symptom; "the handler never compares the invoice owner with the caller" is a cause, and it tells the engineer where to look.
- **Evidence** is a list of verifiable observations (log lines, requests, test results), not opinion. Keep it to the minimum that proves the point.
- **Never paste live secrets.** A report is read by many people and stored for years. Write `password=[redacted]`. A report that leaks a credential is a second incident.
- **Remediation is specific.** "Fix it" is not a remediation; "compare `invoice.owner` with the session user on every request" is.
- **The severity** comes from your scoring, as in [triage](/learn/ethical-hacking/triage-severity).

Reports are also an injection target: if a title or note comes from untrusted data (a log line, a user name) and contains a line break, it can forge a whole fake section such as `SEVERITY: Info`. A renderer must flatten line breaks in every value.

## Two jobs

`render_report` writes the structured text from data, safely. `lint_report` grades a report that a human wrote and returns a bitmask of what is wrong, the structured check a review process can run on every report before it goes out.
