# Security model

Current state as of 30 October 2026. Baremetal runs untrusted code from anonymous visitors, so runner isolation and the sandbox outweigh everything else. Production now requires a separate runner socket service; systemd host verification is still pending. The original audit plan and fix plan are in `docs/archive/` (`security-audit-plan.md`, `security-remediation-plan.md`); the IDs below (L1, R1 ...) match them. Host-level setup is in `docs/deployment-hardening.md`.

## Assets and attackers

Assets: the web server host (shared kernel with the separately configured runner service), MongoDB, accounts (scrypt hash, TOTP secret, sessions), learner data, XP/leaderboard integrity, hidden tests and reference solutions, the `ANTHROPIC_API_KEY` budget, `AUTH_SECRET`. Attackers: anonymous visitor, signed-in learner, a learner with a stolen password, a cross-site page, a hostile network position between proxy and app. Out of scope: host OS hardening beyond the app, third-party services, legal review.

## What is in place

**Sandbox (`src/lib/runner-engine.ts`).** bubblewrap with no network, `--disable-userns`, an x86-64 seccomp denylist (ptrace, bpf, io_uring, userfaultfd, perf_event_open, mount family, kexec, module loading, namespace calls, `fsopen`/`fsmount`/`move_mount` and similar), rlimits, 5 s timeout, output caps. Compilation runs on size-capped private tmpfs with read-only sources; only the executable is returned, over a pipe capped at 64 MiB. Run directories are read-only; `/tmp` and `/dev/shm` are capped. Queue admission is capped per user and per IP, slots are handed directly to waiters, jobs cancel on request abort, and at most 24 may wait (503 beyond). Runner boot cleans only old workdirs. Production web handlers call a versioned Unix-socket service with bounded requests/responses; the service resolves its own exercise definitions and owns admission and cancellation. The socket accepts only the web execution group; its logs exclude source and identities. Optional per-job cgroup v2 limits via `RUNNER_CGROUP_ROOT` (untested on a real host, see deployment doc).

**Result integrity (L1, L2; R1-R4).** Tests run in the learner's process, so the verdict is not an out-of-process one.
- R1: `testExercise` requires the exact ordered test names the reference solution produces (`expectedTestNames`), fails closed if the reference is unavailable, and requires matched C/kernel RUN/result events and a zero normal exit.
- R2: C/asm results carry a per-run nonce passed in `BM_NONCE`, taken in a `.preinit_array` hook with randomized identifiers and wiped from the environment.
- R3: hidden-source leaks are blocked by a scan and diagnostics located in harness code are redacted. Test binaries run with only themselves in the work dir. The scan (`TAMPER_PATTERNS` plus `TAMPER_PATTERNS_BY_LANGUAGE`, chosen by the exercise language) covers the shared patterns (result protocol, `#include "..."`, `include_str!`, `go:embed`, `.incbin`, `/proc/self`) and, per language:
  - Rust: the identifiers `include`, `include_str` and `include_bytes` anywhere (so `include_str !(..)` and macros that forward the name are caught; a comment containing the bare word "include" is rejected too), and `#[path = ..]`.
  - Assembly: `.incbin`, `.include` and the `*_array` section directives in any case, and `.macro`/`.altmacro`/`.irp`/`.irpc` (no lesson uses them; they can assemble a directive name from parameters).
  - C and kernel: `#embed` and `__has_embed` (C23; GCC 15+ accepts it, so it would read `main.c` with the harness appended). `\binclude\b` is deliberately not used for C.
  - Assembly is also built structurally: the harness is compiled to `harness.o` first, `starter.s` is assembled alone in a separate work dir that holds nothing else, and the objects are linked afterwards. The learner's assembly can't read the harness at build time even if a scan pattern is missed.
  - Rust and C still have the harness appended to the learner's file, so a missed spelling would leak it. Options evaluated for a structural fix (not implemented):
    1. *Source on stdin* (`rustc -`, `gcc -x c -`): no file holds the harness. Tested on the host: `include_str!("/dev/stdin")` and `/proc/self/fd/0` read 0 bytes because the pipe is already consumed, and `#include "/dev/stdin"` is empty. Costs: diagnostics say `<stdin>`/`<anon>` instead of `main.rs`/`main.c` (needs a `#line` prefix for C and a rewrite step for rustc), `exec` has no stdin for compile steps yet, and rustc under bubblewrap needs `/proc` for sysroot detection (check before adopting). Most promising.
    2. *Separate units*: compile the learner's code and the harness separately and link. This needs the harness to call learner functions through declared symbols; most C harnesses do, but Rust harnesses use private items and `#[cfg(test)]` modules, so it would mean rewriting lessons.
    3. *Scan only* (current): cheap, but every new spelling is a bypass until the pattern is added, which is how the Rust spacing and macro, asm upper-case and C23 `#embed` variants were found. `fixtures-forgery.mts` holds each of them.
- R4: first-pass XP is limited by an atomic per-user pace and a rolling 24 h cap; minimum solve time applies only with a known start. Suspect passes are saved without XP (`xpWithheld`, flag `xpPending` on interrupted decisions); review with `npm run flagged-accounts`.

**Auth.** scrypt, hashed session tokens, httpOnly SameSite=Strict cookie, TOTP with replay protection, HMAC backup codes (legacy single-use still accepted), atomic password/OTP attempt reservations, pair-scoped lockouts, PoW CAPTCHA, HIBP and common-password checks, password required to enrol or disable 2FA, email verification at signup (legacy accounts without the flag still work), generic signup/reset responses, fragment-based reset links, encrypted durable email queue. `safeNext` is in `src/lib/redirects.ts`.

**API and tutor.** Type and size checks come before quota. The tutor never sees the reference solution, learner text is tag-escaped, replies are buffered for an excerpt check, newcomers (unverified or very new) draw from a pool that reserves capacity for established users, DELETE is rate limited. Hints are served only by `/api/hint`, which records use; public exercises carry `hintCount` only. Tutor and analytics IP keys and console identities use keyed hashes. Both Markdown renderers pass executable-markup fixtures. Production dependency audit: zero advisories (29 Oct 2026).

**Shared addresses (trade-off).** Runner admission, the tutor IP quota and the auth limits no longer treat one IP as one person. Signed-in runs are admitted per account only, the tutor's per-IP quota applies only to accounts under 24 h old, and sign-up/sign-in per-IP limits are 30 and 60 per 15 min. Abuse from one address is therefore bounded by per-account limits, the dollar budget, the per-email limits and the CAPTCHA rather than by a tight per-IP cap. Anonymous traffic keeps its per-IP limits. Values: `docs/deployment-hardening.md`.

**Proxy.** Production refuses to boot without a valid `TRUSTED_PROXY_HOPS`; short `X-Forwarded-For` chains land in a strict per-process "unknown" bucket (L3). `sameOrigin` trusts `X-Forwarded-Host` only if the proxy strips client copies (L4).

## Open limitations (accepted or deployment-bound)

| Item | State |
|---|---|
| L1 same-process forgery | Medium, accepted. A determined C/asm learner can read the nonce and test names from live memory and print a full valid transcript; Go and Rust have no nonce (only exact names); the kernel guest runs in ring 0 and can write the debug console. R1 forces a complete transcript and R4 bounds XP. Real fix: an out-of-process verdict (driver prints raw values, server diffs against a reference transcript), which means rewriting the lesson tests. Revisit if the leaderboard gains prizes. |
| Runner service deployment | Separate service/client, dedicated systemd users, private network, restricted filesystem and cgroup setup are implemented. Install and verify the units on the real host before public launch. A sandbox escape still reaches the runner uid and the shared kernel; service separation is not a VM boundary. |
| Per-job cgroups | Implemented, but never exercised: the dev workspace exposes cgroups read-only. Needs a memory-bomb check on the real host. |
| CSP | `script-src 'unsafe-inline'` remains. Nonce CSP is incompatible with partial prerendering (Next.js guide); needs a rendering migration. The one inline script is the static theme bootstrap. |
| Email delivery | At-least-once; duplicates possible after a crash. Needs SMTP and the worker scheduled. |
| XP recompute | `recompute-xp` must run with the site stopped; it stops for review if a pending decision has no record. |
| Targeted read-only review | Claude reviewed `achievements.ts`, `rhythm.ts`, `inline-script.tsx` and workspace code. Clearing tutor history bypassed the Unaided check; fixed with a sticky progress flag and an API regression check. Anonymous hints, review-reset farming and goal/timezone changes affecting rhythm remain honour-system limitations. This was not a full penetration test. |
| Optional hardening | `__Host-` cookie prefix, session idle timeout and session list with revoke, seccomp allowlist instead of denylist. |

## Regression checks

- `npm run test:e2e -- http://localhost:3100` (auth, 2FA, lockout, CAPTCHA, reset, authorization, redirects) against `npx next start -p 3100`.
- `scripts/security/`: `integrity/fixtures-forgery.mts` (forged and partial transcripts, harness leaks, all languages), `xp-atomic.mts`, `api-hardening.mts` (flood, quota, hint, deletion), `runner-cancellation.mts`, `markdown.mts` (renderer fixtures; run without the `react-server` condition), `quick-fixes.mts` (run with Node's `react-server` condition), plus hostile C probes and `sandbox.sh`. `scripts/auth-crypto-regression.mts` covers crypto.
- `npm run verify:content`, `npx tsc --noEmit`, `npm run lint`, `npm run build`.
- Runner transport checks: `npm run test:runner`; isolated auth, budget, API and browser suites: `npm run test:regressions` (builds an isolated production snapshot). Host verification: `scripts/runner/host-checks.mts --disposable` on a disposable deployment copy as root.
- Sandbox hostile-program tests belong on a disposable VM or cgroup, never a shared host.

## If re-auditing

`docs/archive/security-audit-plan.md` holds the work packages (sandbox, integrity, auth, authorization, injection, tutor, HTTP, deployment, dependencies, privacy) and the severity scale; use it as the checklist, and record findings with a regression `check()` each.
