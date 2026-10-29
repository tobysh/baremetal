---
title: Find shadowed firewall rules
summary: Audit an ordered rule list for rules that can never match because an earlier rule already covers them.
minutes: 45
difficulty: hard
tags: security, detection, network, firewall
id: sec-detection/shadowed-rules
mode: stdout
---
Firewalls break quietly. An administrator adds `deny tcp 10.1.0.0/16 any 80` at the bottom, believing it blocks that network, but a rule above it already says `allow tcp 10.0.0.0/8 any any`. Every packet the new rule would match is taken by the earlier one, so the new rule is shadowed: it is dead text, and the intended protection does not exist. Shadowing is easy to introduce and invisible in normal traffic, which makes it an ideal job for a tool.

Rule A shadows a later rule B when every packet matching B also matches A. Check each field: A's protocol is `any` or equal to B's; A's source network contains B's source network; the same for destinations; and A's port range contains B's range. Network containment with prefix lengths is `lenA <= lenB` and `(netB & maskA) == (netA & maskA)`. Reuse the parser from [the previous lesson](/learn/sec-detection/firewall-rules), which is in your starter.

When the shadowing rule has the same action as the shadowed one, the dead rule is merely redundant. When the actions differ it is a conflict: someone's intent is not being applied, and that is the finding worth a ticket. This lesson reports only shadowing by a single earlier rule; a rule covered by the union of several earlier rules is a harder problem we leave out.
