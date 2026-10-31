---
title: "Capstone: investigate the Glimmerfen incident"
summary: Turn synthetic auth and web logs, file snapshots, an identity graph and audit records into a cited, time-ordered set of confirmed findings.
minutes: 75
difficulty: hard
tags: incident-response, capstone, investigation
id: sec-incident-response/capstone-investigation
mode: harness
---

Glimmerfen Ltd is invented, and so is everything that happened to it. You are handed the records an on-call responder would collect: an authentication log, a web log, file manifests from before and after, an identity graph before and after, and an audit trail of group changes. Somewhere in them an account may have been taken over. Or nothing happened at all.

Your report has three parts. **Findings** are the conclusions you will stand behind, each with the evidence IDs that justify it. A **timeline** lists exactly the evidence you cited, in true UTC order. **Assumptions** are the places where you went beyond what the records say.

Most of the work is deciding what *not* to report. The data contains plausible decoys: a user who mistyped a password, a scanner that never got in, a package update, maintenance that happened before the intrusion, a legitimate administrator granting access, a big download by the report owner. A finding with a decoy in it, or a conclusion with a missing or wrong citation, fails. So does inventing a breach in a quiet case.

This capstone combines the earlier lessons: timestamps with offsets (timeline), tying actions to accounts (correlation), file diffs with timestamp tampering (snapshot diff), and new privilege paths with audit attribution. Credential spread and verification are practised separately, so this lesson stops at confirmed findings.

**What the machine can and cannot check.** The tests check findings, citations and ordering against an authored answer key. They cannot judge your written assumptions; those are for a human reviewer. Rules below are deliberately explicit, so this exercises careful application of criteria and citation hygiene, not open-ended detection.

**Limitation:** all clocks are assumed synchronized, and the records are assumed complete and unaltered.
