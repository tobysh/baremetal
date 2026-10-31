Parse every Auth and Web line into UTC Unix seconds first, and keep the ID next to each record. Everything else is lookups.
===
Find the initial access before anything else: persistence, privilege and exfiltration all depend on its time T and, for privilege and exfiltration, on who and which address it was.
===
For the graph, compute "reaches admins" by walking edges backwards from admins, once on Before and once on After.
===
Each decoy fails exactly one condition: wrong window, wrong actor, time before T, non-autostart path, allowlisted hash, or too few bytes. Re-read the rule list against each suspicious record.
===
Cite only records that satisfy the rule: the timeline is the union of your citations, so a stray citation puts a wrong event in the evidence package.
