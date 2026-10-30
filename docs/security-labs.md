# Synthetic security investigation curriculum

The Security Investigation Labs course (`sec-synthetic-labs`, Go) is discovered automatically by the curriculum loader. It follows Go Practice and adds nine lessons without changing published IDs. Each has a compiling starter, a reference solution, progressive hints and hidden tests. Breaking and Fixing C adds `sec-c-bugs/stack-control` as a companion lesson.

| Phase | Lesson | Input and validation |
| --- | --- | --- |
| 1 | Capture evidence | JSON capture dump; vary ports, flow endpoints, handshake completeness and HTTP markers; deduplicate DNS queries. |
| 1 | Subnets and routes | Seeded address cases across every IPv4 prefix length; /0, /31, /32, invalid inputs and longest-prefix route ties. |
| 1 | Linux permissions | Checked-in fictional tar; generated archives vary names, modes, owners, regular files, directories and symlinks; compare sorted findings. |
| 2 | Query payload | In-process concatenating query target; assert unauthorized rows across different datasets and no such result from bound values. |
| 2 | Weak cookie | Known-message HMAC dictionary recovery; varying secrets, candidate positions and requested admin bodies; malformed/no-match cases. |
| 2 | Browser policy quiz | Authored explanations and a fixed answer map for CSRF, CORS, HttpOnly and session lifecycle. |
| 3 | Scan correlation | Static scan text plus fictional LAB advisory table; varying ports/versions, duplicates, invalid ports, closed/filtered/UDP decoys. |
| 3 | Privilege paths | JSON permissions, cron and sudo state; permission class selection, root execution, group membership and ineligible rules. |
| 3 | Directory graph | BloodHound-inspired normalized JSON; cycles, directed edges, varying chain lengths, shortest paths, deterministic ties and malformed graphs. |
| 3 | C stack control | Deterministic byte-array frame with a saved-return slot; varying offsets and addresses, binary zero bytes, capacity guards and a repaired copy. |

## Runner choices and limits

Go is already supported by the editor and isolated runner. Python and Bash are not currently lesson languages. These labs use Go functions to analyze supplied data rather than introducing another execution path. Linux lessons inspect archive metadata and JSON state; they do not run Bash against an extracted filesystem. The privilege model deliberately excludes directory replacement, ACLs and command-specific sudo escapes.

The C companion is a model of control-flow overwrite, not a native return-address exploit. It does not disable ASLR, stack protection or other mitigations and does not invoke C undefined behavior. A real compiled stack-overflow target remains future work requiring a pinned toolchain and explicit target layout.

Lesson assets are checked in beside each lesson. The renderer does not currently provide arbitrary file downloads or mount content files in learner programs. Public JSON examples are therefore included in task Markdown; hidden Go harnesses pass fresh captures, tar bytes and graph data directly to learner functions. The tar task describes its public archive findings. No fixtures need network access. The JSON capture format is a normalized lesson format, not a Wireshark export schema, and the directory format is inspired by BloodHound rather than compatible with its importer. Advisory IDs are fictional LAB identifiers, avoiding inaccurate real-world CVE claims.

## Validation

Run `npm run verify:content -- sec-synthetic-labs` and `npm run verify:content -- sec-c-bugs`, then `npm run check:ids`. The content verifier checks solutions pass and starters compile but fail. Hidden Go tests use fixed seeds and stable names so the runner can compare its expected test transcript. Tests run through the existing sandbox and retain its documented same-process verdict limitations.
