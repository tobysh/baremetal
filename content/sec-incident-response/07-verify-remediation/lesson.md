---
title: Verify that remediation worked
summary: Check each finding against a post-remediation snapshot, report closed, open or unverifiable, and watch for attacker activity after the fix.
minutes: 35
difficulty: hard
tags: incident-response, remediation, verification
id: sec-incident-response/verify-remediation
mode: harness
---

"We deleted it" is a claim; a fresh observation after the change is evidence. Verification asks, for every finding, whether the post-fix state shows it gone, and says plainly when it cannot tell.

Three honest outcomes exist. **Closed**: a snapshot taken after the remediation shows the artifact absent, or the credential rotated and the old one rejected. **Open**: the snapshot still shows it. **Unverifiable**: the snapshot predates the fix (a clean old snapshot proves nothing about now), it does not cover that kind of artifact, or it has no data for that credential.

A credential finding needs two checks. A new version alone is not enough if the old secret still works; a rejection of the old one alone is not enough if you cannot show the replacement exists.

Finally, a clean snapshot does not mean the intruder is out. Activity from known indicators (here, addresses) *after* the remediation time suggests a path you missed, even if every listed finding is closed.

**Limitation:** the snapshot and probes are inputs. Real verification must also trust how they were collected.
