---
title: "Capstone: assess, fix, report"
summary: Run a scoped assessment of a simulated host from recon to findings, repair the access-control hole you find, and write the report.
minutes: 60
difficulty: hard
tags: capstone, recon, reporting, access-control
id: ethical-hacking/capstone-assessment
mode: harness
---
> **Simulated target.** Everything in this lesson is made up and runs inside the sandbox: the host, its ports, banners and documents are test data behind function pointers; the written authorization is a struct field. In scope: the port range named in the policy and the documents it lists. Nothing here is a real system. Never use a technique from this course on a system you do not own or do not have written permission to test. Start with [Law, ethics and permission](/learn/ethical-hacking/law-and-ethics) if you have not yet.

This capstone is a small, complete engagement, the loop every assessment follows: **authorization, recon, findings, fix, re-test, report**. Each step reuses a lesson.

1. **Authorization** ([law and ethics](/learn/ethical-hacking/law-and-ethics)). The `Policy` carries the signed scope: `authorized`, and the port range `scope_lo..scope_hi`. If `authorized` is 0, `assess` returns `-2` and touches nothing, not even a probe. It never probes outside the range.
2. **Recon** ([scanner](/learn/ethical-hacking/port-scanner), [banners](/learn/ethical-hacking/banner-identification)). Scan the range (three attempts per port at most), read each open port's banner once, and compare with the allow-list and the advisory list.
3. **Findings.** In discovery order (ascending port; for a port, `unexpected` first, then the banner finding), then the access-control test:

   | kind | meaning | severity |
   | --- | --- | --- |
   | `K_UNEXPECTED` (1) | open port not on the allow-list | Medium (2) |
   | `K_OUTDATED` (2) | banner `product/version` older than an advisory's `fixed_in` | High (3) |
   | `K_UNVERIFIED` (3) | banner is a bare product with an advisory (version hidden) | Low (1) |
   | `K_IDOR` (4) | a document owned by someone else was readable by the tester | Critical (4) |

   The access test ([IDOR](/learn/ethical-hacking/idor)): for every policy document whose owner is not `tester_uid`, call `get_doc` once as the tester. If any answers 200, add **one** finding for the first leaking document (`port` 0, evidence naming the document). Documents the tester owns are never requested. The final list is sorted by severity, highest first, **stably**.
4. **Fix.** `doc_fixed` is the repaired handler: the owner gets `200` and the text, everybody else (and nonexistent ids) gets `404`. The tests re-run your assessment against a target that uses your fix, and the access finding must be gone.
5. **Report** ([finding report](/learn/ethical-hacking/finding-report)). `report_text` renders `REPORT: N findings`, a blank line, then each finding block in the standard structure, separated by blank lines.

Every finding needs a non-empty title, root cause, evidence and remediation. The evidence must contain the decimal port (`port 8080`) or `doc 1043`, so a reader can verify it.
