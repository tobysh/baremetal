---
title: Check password candidates
summary: Apply a small, explainable password policy: length, character classes, common strings and repeats. Report every violated rule per candidate.
minutes: 20
difficulty: easy
tags: security, passwords, input-validation
id: sec-crypto/password-policy
mode: stdout
---
Current guidance for passwords puts length first. A longer secret is harder to guess than a shorter one with a few punctuation marks, and blocking passwords that appear in breach lists and common-word lists helps more than complicated composition rules. Rules should also be explainable: a rejected user deserves to know which rule failed. Servers must check these policies themselves, since a browser check can be skipped, and must store only a salted slow hash of the accepted password.

This lesson implements a small teaching policy over candidate passwords read one per line. It is a simplified policy, not an endorsement of any particular rule set, and a real deployment should consult current guidance and check against large breach corpora. The input is a file of made-up candidates; none is a real secret.

Count bytes for the length. For character classes count how many of lowercase letters, uppercase letters, digits and everything else appear. Print every rule that fails, in the fixed order given in the task, joined by commas, or `ok` if none fail.
