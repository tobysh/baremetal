# Security remediation plan — Baremetal

Result of the four-area review (auth, sandbox, API/web, tutor/data). It was a read-only code review: nothing was run, and every finding below still needs a proof-of-concept before the fix is declared done. `docs/security-audit.md` is the audit plan (L1–L10); this document is the fix order. "Ref" gives the L-number it extends.

## Order of work

| Phase | Theme | Why first | Effort |
|---|---|---|---|
| 1 | Result integrity (R1–R3) | The only High findings; the leaderboard and seals are meaningless until fixed | 2–4 days |
| 2 | Deployment guards (D1–D3) | One config mistake defeats every IP-based limit | 0.5 day |
| 3 | Quick fixes (Q1–Q8) | Small, independent, low risk | 1 day |
| 4 | Runner resources (S1–S3) | Availability; needs service-manager changes | 1–2 days |
| 5 | Auth and account hardening (A1–A7) | Low/medium, no known exploit in the wild | 1–2 days |
| 6 | Defence in depth (H1–H5) | Cheap, but must not break the site | 1 day |

Each item lists the change, the files, and how to verify it. Do the phases in order, one PR per phase, and run `npm run verify:content`, `npx tsc --noEmit`, `npm run lint` and `npm run test:e2e` before each merge.

---

## Phase 1 — Result integrity (High; Ref L1, L2)

**Problem.** The verdict is computed from the learner process's own output. The tamper scan (`harnessTampering`, `runner.ts:477-483`) matches literal text, so `__attribute__((__constructor__))`, runtime-built strings, or reading the nonce out of the test binary's `.rodata` all bypass it. Go and Rust have no nonce. `allPassed` (`runner.ts:586, 624, 654`) never compares against the harness's real test list, so one PASS then `exit(0)` passes a lesson.

### R1 — Completeness check (do first; closes most of the cheap forgeries)
- Compute the expected test set per exercise at load time in `src/lib/curriculum/`: parse test names from `tests.*` (Go `func Test…`, Rust `#[test] fn …`) and count `CHECK(` calls for C/asm/kernel. Cache it on the exercise.
- In `parseHarnessOutput` and the Go/Rust/kernel equivalents, require every expected name or index exactly once, in order, and no unexpected extras. Otherwise `passed = false` with the message "tests did not run to completion".
- For `cases.json` lessons (stdin/stdout diffing), no change; the server already decides those.
- Verify: add `scripts/` fixtures with a forged-PASS-then-exit solution per language and assert they fail. Keep them out of the lesson `solution.*` files. `verify:content` must still pass for all real solutions.

### R2 — Results channel the learner cannot write to
- C/asm/kernel: have `CHECK` write results to a dedicated fd (for example fd 3, `O_APPEND`, a pipe or file the server reads) instead of stdout. Pass the nonce via that fd's protocol only, never as a string in `.rodata`; derive it at runtime from a value passed in the environment and deleted from `environ` before learner code can run (`main` wrapper in the hidden harness, with learner code compiled as a separate object so its constructors still run first, so also bind the check to R1).
- Caveat: learner code in the same process can still `write(3, …)`. R1 plus a harness-side running hash (each result line chained to the previous plus the nonce) makes forging a full valid transcript require reading the nonce at runtime, which `keepOnlyProgram` and the environment scrub make hard but not impossible. Record this as an accepted residual in `security-audit.md` L1.
- Go/Rust: run the learner package and the tests as separate steps if feasible, for example the learner's `main.go` compiled into a package under test, with the test driver in its own process that imports it, so `init()` in learner code can still run but cannot see the driver's output stream. If that costs too much, rely on R1 (names parsed from the harness, so forged names must match exactly) and R3.
- Kernel: results are forgeable by design (ring 0 guest). Accept the gap, apply R1, and add a server-side check that the exit came through `isa-debug-exit` after the expected number of results. Note it in the findings log.

### R3 — Remove compile-time leaks of the hidden tests (L2)
- Compile the learner file alone first (object file, Go package archive, `rustc --emit=obj`) in a directory containing no harness. Add `tests.*` only at the link or `go test -c` step, in a second sandbox call.
- Until then, extend the scan to deny in learner code: `//go:embed`, `include_str!`, `include_bytes!`, `include!`, asm `.include` / `.incbin`, and `#include "…"` with a quoted path (system headers in angle brackets stay).
- Verify: fixtures that try each of the above and assert the output never contains a line from the hidden harness.

### R4 — Make forged XP cheap to undo
- Add a per-day first-pass XP cap next to `PASS_PACE` (`progress.ts:35-60`) and flag passes faster than a per-difficulty floor (`xpWithheld`, already honoured by `recompute-xp`).
- Add an admin view or script listing flagged accounts, so cheating can be reviewed and reversed.

---

## Phase 2 — Deployment guards (Ref L3, L4)

### D1 — `clientIp` fallback (`rate-limit.ts:44-51`)
- When the `X-Forwarded-For` chain is shorter than `TRUSTED_PROXY_HOPS`, return the socket address or `"unknown"`, never `chain[0]`.

### D2 — Refuse to start misconfigured
- In production (`NODE_ENV=production`), fail fast at boot if `TRUSTED_PROXY_HOPS` is unset. Log the value at startup.
- Treat the `"unknown"` bucket as per-process and strict, so a missing header cannot become a shared allowance that other users deplete.

### D3 — Document the proxy contract
- In `docs/deployment-hardening.md` and README: the proxy must overwrite (not append) `X-Forwarded-For`, drop client `X-Forwarded-Host`, and the hop count must match. Add a concrete MongoDB-auth how-to (a dedicated `baremetal` user with `readWrite` on one database, bind to loopback) and a note that `recompute-xp` must run with the site stopped.

---

## Phase 3 — Quick fixes

| ID | Change | File | Verify |
|---|---|---|---|
| Q1 | Tutor: `typeof body.code === "string"` and `lastOutput`, 16 KB cap each, 400 on failure (`code` currently only gets the 128 KB `readJson` cap). Check before consuming quota. | `api/tutor/route.ts:42-43, 62-90` | e2e: object body returns 400, not 500, and no quota used |
| Q2 | Tutor: stop sending the reference solution. Replace with an authored `notes` field (or the test intent) in the lesson, falling back to nothing. Escape `</current_code>`-style tags in learner content. | `tutor.ts:53-63` | Injection prompt can no longer recover solution lines; add a reply filter (n-gram overlap against the solution) before streaming and caching as a second layer |
| Q3 | Tutor: require an account at least N minutes old (or verified, after A5) and keep a quota reserve for established users, so a signup flood cannot exhaust the global cap. | `api/tutor/route.ts` | e2e |
| Q4 | `safeNext`: parse with `new URL(next, "http://x")`, require origin `http://x`, reject control characters and backslashes, return `pathname + search`. | `auth.ts:202-205` | Unit-style checks in `e2e-auth.mts` for `/%09/evil.com`, `/\evil.com`, `//evil.com`, `/ok?x=1` |
| Q5 | Rate-limit `/api/tutor` DELETE. | `api/tutor/route.ts` | e2e |
| Q6 | Run queue: cap in-flight jobs per user and per IP at 2, cancel on `req.signal`, add a queue wait timeout. Fix the `active--` / `queue.shift()` race by handing the slot directly to the waiter. | `runner.ts:48-65`, `api/run/route.ts:23-28` | Fire 30 parallel runs as one user; others still get slots |
| Q7 | Wire `cleanupStaleWorkdirs` to boot only, removing `bm-run-*` older than 15 minutes. | `runner.ts:658-661` | Does not touch a live run's dir |
| Q8 | Hide hints from the page payload. Serve them from `/api/hint` which records `hintsUsed` as it returns them, so the "unaided" seal means something. | `toPublicExercise`, `api/progress/route.ts` | The page source contains no hint text |

---

## Phase 4 — Runner resources (Ref L5, L6)

- **S1 — cgroup limits.** Run each job under a systemd scope or cgroup v2 slice (`MemoryMax`, `TasksMax`, `CPUQuota`, `IOWeight`), for example `systemd-run --user --scope -p …` around `bwrap`, or a template unit. Cover the Go compile (no `--as` today) and the QEMU run. Document the unit in `docs/deployment-hardening.md`. Verify with a memory-bomb fixture that is killed without OOMing the host.
- **S2 — Disk.** Put each job's work dir on a size-capped tmpfs (or project quota) for the compile phase too, not just a per-file `fsize`.
- **S3 — Seccomp gaps (defence in depth).** Add `fsopen`, `fsmount`, `fspick`, `move_mount`, `open_tree`, `mount_setattr`, `kcmp`, `pidfd_getfd`, `ioperm`, `iopl`, `modify_ldt`, `clock_adjtime` and `memfd_secret` to the denylist in `runner.ts:72-93`. Longer term, move to an allowlist and run the sandbox as a dedicated uid with no access to `.env.local` or `.data/db` (L6).
- Also: cache `nprocLimit()` instead of scanning `/proc` on every exec (`runner.ts:34-46`).

---

## Phase 5 — Auth and account hardening (Ref L8, L10)

| ID | Change | File |
|---|---|---|
| A1 | Require the current password to start 2FA enrolment (as `disableTwoFactor` already does) and email the owner when 2FA is enabled or disabled. This stops a hijacked session locking out the owner. | `settings/actions.ts:119-159` |
| A2 | Clear login failure counters only after the full login (password and second factor). Count OTP failures per user across pending sessions so backoff survives. | `(auth)/actions.ts:143-145` |
| A3 | Increment the failure counter atomically before verifying the password, and refund on success, to close the parallel-guess race. | `security.ts:185-217`, `actions.ts:118,137` |
| A4 | Lock per (email, IP) pair, or allow a valid password plus captcha through, so five bad guesses by a stranger don't lock the owner out. | `security.ts` |
| A5 | Email verification at signup (token flow modelled on password reset). Then use "verified" in Q3. | `(auth)/actions.ts:77-94` |
| A6 | Signup returns one generic response and emails an existing owner; forgot-password queues the email instead of awaiting it. | `actions.ts:97-98, 224-247` |
| A7 | Backup codes: HMAC with the server key (`hmac()` in `crypto.ts`) or at least 12 characters, with a migration that rehashes on next use. Move the reset token out of the query string where practical (L10). | `totp.ts:77-88` |

Also consider: a `__Host-` prefix on the session cookie, an idle timeout, and a sessions list with revoke.

---

## Phase 6 — Defence in depth (Ref L9)

- **H1 — CSP nonces.** Replace `script-src 'unsafe-inline'` with a nonce-based CSP. This forces dynamic rendering for affected pages, so measure first and apply to the highest-risk routes. Review `inline-script.tsx` (unread so far).
- **H2 — Logging.** Hash or truncate the email and IP in failed-login logs (`security.ts:34`), or document log retention in `content/legal/privacy.md`. Update the legal pages if any stored data or key changes (project rule).
- **H3 — Rate-limit keys.** Hash IPs in `tutor:ip:*` / `analytics:*` keys, or correct the "few minutes" retention wording.
- **H4 — Markdown.** Confirm the tutor reply renderer (`MiniMarkdown`) and lesson renderer cannot emit raw HTML or `javascript:` links. Add a test fixture.
- **H5 — Dependencies.** Re-run `npm audit --omit=dev` yourself, use `npm ci` in deployment, and review the caret ranges before release.

---

## Not yet reviewed

`src/lib/achievements.ts`, `src/lib/rhythm.ts`, `inline-script.tsx`, and the client workspace code were not read. Review them as a follow-up pass, mainly for XP or seal bypasses and XSS sinks.

## Exit criteria

- Every High item has a failing proof-of-concept fixture before the fix and a passing one after.
- `npm run verify:content`, `npx tsc --noEmit`, `npm run lint` and `npm run test:e2e` all pass.
- `docs/security-audit.md` §4 (findings log) is filled in from this document, and L1 is re-rated with the accepted residual stated.
- A re-review by a fresh agent finds no open High or Medium item, or each remaining one is recorded as an accepted risk with an owner.
