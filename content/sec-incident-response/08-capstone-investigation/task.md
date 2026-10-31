Implement `Investigate(c Case) Report`.

**Inputs.** `Auth` lines: `A<n> <RFC3339> user=<u> ip=<ip> ok|fail`. `Web` lines: `W<n> <RFC3339> ip=<ip> user=<u|-> bytes=<n> <path>`. Timestamps carry arbitrary offsets. `Base` and `Now` are file manifests (`Path, Hash, Owner, Mode, MTime, CTime`), `Allowed` maps a path to a trusted update hash. `Before` and `After` are `MemberOf` edges, `Audit` entries are group additions `{ID, Time, Actor, From, To}`. The privileged group is `admins`.

**Findings** (Kind, Subject, Cites):
1. `initial-access`: a successful login from an IP with **at least 5 failed attempts from that same IP in the 600 seconds before it** (`[t-600, t)`, any usernames). Subject is the user. Cites: the success ID and the in-window failure IDs. `T` is the success time.
2. `persistence` (only if `initial-access` found): a file in `Now` that is new or whose hash differs from `Base`, whose hash is not the allowlisted one, under `/etc/cron.d/` or `/etc/systemd/system/` or named `.../.ssh/authorized_keys`, with **CTime >= T**. Use CTime: MTime can be forged. Subject is the path. Cite `"F:"+path`.
3. `privilege-change`: an `Audit` entry whose Actor is the compromised user, `Time >= T`, whose edge is in `After` but not `Before`, whose `To` is `admins` or reaches `admins` in `After`, and whose `From` could **not** reach `admins` in `Before`. Subject `From>To`. Cite the audit ID.
4. `exfiltration`: web requests from the attacker IP at or after `T` with path starting `/export/`; if their bytes sum to at least 1,000,000, Subject is the IP and Cites are those request IDs.

Return findings sorted by Kind then Subject (plain string order), no duplicate cites. `Timeline` is the union of all cited IDs sorted by UTC time then ID; file evidence is timed at its CTime and audit evidence at its Time. Add at least one entry to `Assumptions` when you report anything (not machine-graded). With no `initial-access` there are no findings and an empty timeline.

Public example: five failures from one IP within a minute, then a success for `kim`, give one `initial-access` finding for `kim`. Hidden cases vary users, addresses, offsets, boundary timings (exactly 600 s), forged mtimes, and whether persistence, key tampering, a two-step grant or exfiltration happened.
