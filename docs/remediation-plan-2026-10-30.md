# Remediation plan — 30 October 2026

Audience: agents (and people) fixing the findings below. Dev environment, nothing deployed.
Each work item is self-contained: read its **Evidence**, make the **Change**, run its **Verify** step.
Read `CLAUDE.md`, `AGENTS.md` and `HANDOFF.md` first. Do not commit, push or run `recompute-xp`
unless the item says so. Never add Claude attribution lines to commits (user rule in memory).

## Baseline when this review was done

| Check | Result |
| --- | --- |
| `npx tsc --noEmit` | clean |
| `npm run lint` | 0 errors, 1 warning (`goExtra` unused, `scripts/security/integrity/poc-fakepass.mts:19`) |
| `npm run check:ids` / `check:theme` / `check:learning` | pass (ids: "7 new id(s) not in snapshot") |
| `npm run build` (prod env vars, throwaway DB name) | pass, one Turbopack tracing warning |
| `npm audit --omit=dev` | 0 vulnerabilities |
| `npm run verify:content` | pass: all 430 lessons verified (solutions pass, starters compile and fail) |

The codebase is in better shape than the findings list suggests: there is no failing check. The
findings are about risk concentration, request-size handling and a few correctness gaps. Items are
ordered by priority; do P0 first because everything else is easier to review once it is done.

---

## P0 — Work exists only in the working tree

**Evidence.** `git status` shows 98 changed files plus ~26 untracked source files, nothing committed
since `124eb58 "fixing actions maybe"`. `src/lib/runner.ts` went from ~906 lines to a 4-line shim
(`git diff --stat`) and now depends on untracked `runner-engine.ts`, `runner-client.ts`,
`runner-protocol.ts`, `runner-types.ts`. Also untracked: `.github/actions/toolchains/`,
`deploy/systemd/`, `scripts/runner-service.mts`, `scripts/runner-host.mts`, `scripts/runner/`,
`scripts/checks/`, `scripts/browser/`, `playwright.config.ts`, `src/lib/learning*.ts`,
seven new `content/sec-appsec/08..14-*` lessons, `docs/learning-validation.md`.
The working-tree `ci.yml` uses a composite action and scripts that are untracked, so it cannot run
from a push as the tree stands. (Whether the committed `ci.yml` passes on GitHub was not checked; the
last commit message, "fixing actions maybe", suggests it did not.) One disk failure or stray `git checkout .` loses the
recovery work.

**Change.**
1. Run `npm run check:ids -- --update` so the 7 new sec-appsec ids enter the snapshot (they are
   additions only; confirm the diff of the snapshot file shows no removals).
2. Commit in logical groups, not one blob. Suggested order:
   a. runner split (`runner*.ts`, `scripts/runner*`, `deploy/`, `scripts/runner/`, `api/run`, `instrumentation.ts`)
   b. CI + guards (`.github/`, `scripts/checks/`, `scripts/check-*.mts`, `playwright.config.ts`, `scripts/browser/`)
   c. learning-path/cohort code and docs (`src/lib/learning*.ts`, `docs/learning-validation.md`)
   d. content: sec-appsec lessons, track.json edits, `cases.json`/solution edits
   e. UI, legal pages, remaining docs
3. Before each commit run `npx tsc --noEmit && npm run lint`. Use plain messages with no attribution trailer.
4. Do not push. Leave that to the owner.
5. Stale branches/worktrees: `sec/phase-1-result-integrity`, `worktree-agent-a6a88478…`,
   `…aa5cbc13…`, `…ad9bdcf4…` each carry one commit not on `main`. HANDOFF says their content was
   recovered into the tree. For each, run `git diff main..<branch> --stat` and confirm every file is
   present in the working tree before removing anything. Delete worktrees only after that, and only
   with the owner's go-ahead (`git worktree remove`, then `git branch -d`; `-d` refuses unmerged
   branches, which is the safety net — do not use `-D`).

**Verify.** `git status --short` is empty (or only intentional leftovers); `npm run build` still passes
from a clean checkout of the new HEAD (`git stash -u` is not needed — use `git worktree add` on HEAD in
the scratchpad, `npm ci`, `npx next typegen`, `npx tsc --noEmit`).

---

## P1 — Request bodies are buffered before any limit applies

**Evidence.** `src/lib/http.ts` `readJson` checks `Content-Length` only, then `await req.text()`.
A chunked request (no `Content-Length`) is read in full before the `text.length` check. It also measures
UTF-16 length, not bytes. Callers: `/api/run` (anonymous allowed, rate limit runs *after* the body is
parsed, `src/app/api/run/route.ts:19-30`), `/api/hint` (anonymous), `/api/analytics` (anonymous,
intended cap 4096), `/api/tutor`, `/api/progress` (signed in).

**Change.**
1. Rewrite `readJson` to read `req.body` as a stream, count bytes, and stop (cancel the reader and
   return `null`) as soon as the cap is exceeded. Keep the `Content-Length` fast path. Keep the
   signature so no caller changes.
2. In `/api/run`, move the rate-limit check ahead of `readJson`. The key needs only the user and the
   IP, both available before the body. Keep the exercise-specific checks after.
3. Same reordering for `/api/hint` and `/api/analytics` (analytics already limits before use but
   after `readJson`; move it).

**Verify.** Add a case to `scripts/e2e-auth.mts`: POST a ~5 MB body with `Transfer-Encoding: chunked`
to `/api/hint` and `/api/analytics`; expect 400/204 quickly and flat server memory. Add a unit-style check
that a 4097-byte multi-byte body is rejected for analytics. Run
`npx next start -p 3100` then `npm run test:e2e -- http://localhost:3100`.

---

## P1 — `sameOrigin` trusts forwarded headers unconditionally

**Evidence.** `src/lib/http.ts`: `x-forwarded-host` is preferred over `host` whenever present, and
`origin === SITE.url` is accepted. A missing `Origin` falls back to `Sec-Fetch-Site`. Cookies are
SameSite=Strict, so impact is limited to cross-site anonymous POSTs (run, hint, analytics), but the doc
(`security.md` L4) makes this a deployment assumption that nothing enforces.

**Change.** In production, compare the `Origin` host only against the host of `SITE.url`
(`NEXT_PUBLIC_SITE_URL`); in dev keep the current host-header behaviour so LAN/phone testing works
(`allowedDevOrigins` already exists). Do not read `x-forwarded-host` in production.

**Verify.** e2e: request with `Origin: https://evil.example` and `X-Forwarded-Host: evil.example`
returns 403 in a production-mode server (`NODE_ENV=production`, set `NEXT_PUBLIC_SITE_URL`,
`TRUSTED_PROXY_HOPS=1`, `RUNNER_SOCKET` as in the CI build step).

---

## P2 — Account deletion leaves residue and races with in-flight writes

**Evidence.** `src/app/settings/actions.ts:203-222`: data collections are deleted in parallel and the
`users` document last. An autosave (`PUT /api/progress`, upserts by `userId`) or a tutor turn that is
in flight can re-create `progress`/`tutor_threads` rows for a deleted user. Not removed at all:
`tutor_spend` keys `u:<userId>:<day>` (60-day TTL), `login_failures` (hashed, 24 h TTL).
`/api/account/export` also omits `tutor_spend`.

**Change.**
1. Delete the `users` doc and sessions first (this makes `getCurrentUser` return null, so new writes are
   refused), then delete the dependent collections, then add `tutor_spend` (`{ key: { $regex: ^u:<id>: } }`,
   anchored and escaped) to the list.
2. Add a small `scripts/` sweep or admin command that removes orphaned `progress`, `tutor_threads`,
   `xp_events`, `achievements`, `reviews` whose `userId` has no user. Run it as a dry run first and
   report counts.
3. Update `content/legal/privacy.md` only if retention wording changes (it must stay accurate: spend
   rows are deleted on account deletion after this change).

**Verify.** `scripts/e2e-auth.mts` has no account-deletion case today, so add one: create a user, make progress, trigger deletion, assert every
collection above has zero documents for that user id, then assert a late `PUT /api/progress` with the
old cookie returns 401.

---

## P2 — Inconsistent IP handling in rate-limit keys

**Evidence.** `/api/run` writes `run:ip:${ip}` with the raw client IP into `rate_limits`
(`api/run/route.ts:34`). Every other route hashes it (`hmac("rate-ip", ip)`). The privacy notice
describes hashed network identifiers.

**Change.** Hash it the same way. Search for other raw-IP keys:
`grep -rn 'ip}' src | grep -i rate`. Also `UNKNOWN_IP` is already per-process, leave it.

**Verify.** `grep` shows no raw-IP key remains; the flood suite under `scripts/security/` still passes.

---

## P3 — Smaller items

1. **Turbopack tracing warning** ("Dynamic filesystem access causes tracing of the whole project", build
   log). Likely `runner-engine.ts` building paths at runtime. Narrow the path expressions or mark with
   `/*turbopackIgnore: true*/` per the Next 16 docs in `node_modules/next/dist/docs/`. Check the
   standalone/trace size does not grow before and after.
2. **Lint warning** `goExtra` in `scripts/security/integrity/poc-fakepass.mts:19`: use it or delete it.
3. **CI reproducibility.** `.github/actions/toolchains/action.yml` uses `go-version: stable` and
   unpinned apt `rustc`. Pin the Go version to what the curriculum was verified with (`go version`
   locally). It also disables the AppArmor userns restriction, which is correct for a disposable runner;
   keep the comment saying so, and never copy that line into deploy docs.
4. **`src/lib/auth.ts` import order**: `import { goalOf } from "./rhythm"` sits above `import
   "server-only"`. Confirm `rhythm.ts` is server-safe and move `server-only` to the first line.
5. **Optional auth hardening** (already listed in `security.md`): `__Host-` cookie prefix, idle timeout,
   session list with revoke. Only start these if the owner wants them; they change cookie names and so
   log everyone out.
6. **Seccomp is a denylist** (`runner-engine.ts` `DENIED_SYSCALLS`) and `/api/run` in dev executes under
   the web uid when `RUNNER_SOCKET` is unset. Both are documented, accepted limits. Do not change
   them here; they belong to the disposable-host verification in `deployment-hardening.md`.

---

## Not findings (checked, fine)

- Hidden tests and solutions never reach the browser: pages use `toPublicExercise`
  (`src/lib/curriculum/types.ts:65`), hints go through `/api/hint`.
- XP award is a single atomic pipeline update with pace, daily-cap and too-fast flags
  (`src/lib/progress.ts`); budget reservation (`tutor-budget.ts`) is race-safe via the unique key and a
  conditional upsert.
- Login/OTP attempt reservation is atomic; session tokens are hashed; admin routes require 2FA.
- Runner socket validates every message with zod, caps request and response sizes, refuses to start with
  web secrets in its environment, and refuses to unlink a pre-existing socket.
- The seccomp BPF jump offsets were checked by hand and are correct.

## Needs external evidence (cannot be fixed in code)

Same list as `docs/review-2026-10-29.md`: real-host cgroup/isolation probe, SMTP delivery, off-host
backup restore, beginner playtest, screen-reader pass, GitHub-hosted CI run.
