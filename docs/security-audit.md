# Security audit plan — Baremetal

Draft, 27 October 2026. Scope is the repository at `eeaf1c0` (Next.js 16.4 app, local MongoDB, bubblewrap/QEMU code runner, Claude/Ollama tutor).

The plan is ordered by risk. Baremetal **runs untrusted code from anonymous visitors on the web server's host**, so the sandbox outweighs everything else. Section 1 lists leads I noticed while reading the code for this plan. They are hypotheses to confirm in the audit, not verified findings.

---

## 0. Threat model

| Asset | Threats |
|---|---|
| Host running `next start` (same uid as the sandboxes) | Sandbox escape, resource exhaustion, disk/RAM fill |
| MongoDB (no auth by default, `127.0.0.1:27017`) | Reachable by anything on the host that escapes the sandbox |
| User accounts (email, scrypt hash, TOTP secret, sessions) | Takeover, credential stuffing, 2FA bypass, enumeration |
| Learner data (code, tutor threads, progress) | IDOR, leakage via export or public profiles |
| XP / leaderboard integrity | Spoofed test passes, replay, race conditions |
| Hidden tests, reference solutions | Leakage via the sandbox or tutor prompt injection |
| `ANTHROPIC_API_KEY` budget | Quota bypass, cost amplification |
| `AUTH_SECRET` | Forged CAPTCHA, decrypting TOTP secrets, analytics salt |

Attackers in scope: anonymous visitor, signed-in learner, malicious learner holding a stolen password, a cross-site page (CSRF/XSS), and a hostile network position between the reverse proxy and the app.

Out of scope unless asked: physical/host OS hardening outside the app, third-party services (HIBP, Anthropic), and a full legal review.

---

## 1. Leads to confirm first

| # | Area | Lead | Where | Initial severity |
|---|---|---|---|---|
| L1 | Test integrity | Test results are parsed from the learner process's own stdout (C/asm `@@PASS`, Rust `test __bm_tests::x ... ok`, Go `--- PASS:`) or from port 0xE9 (kernel). Learner code can print fake pass lines from a constructor/`init()` and exit 0 before any real test runs. A kernel can `outb` to 0xE9 and write 1 to `isa-debug-exit`. Result: XP and leaderboard rank for every lesson without solving it. | `runner.ts` `parseHarnessOutput`, `testRustHarness`, `testGoHarness`, `testKernelHarness` | High (integrity) |
| L2 | Hidden tests | In test mode the harness source is in the sandbox work dir (`/work/main.c`, `tests.c`, `bm_test.go`, `main.rs`). Learner code can read it at runtime (`fopen`) or at compile time (`include_str!`, `#include`) and print it into the returned output. | `runner.ts` `cTestSource`, `compileAsm`, `compileGo` | Medium |
| L3 | Rate limiting / IP | `clientIp` trusts `X-Forwarded-For`. If the app is reachable without a proxy that overwrites the header, any client chooses its own IP and bypasses per-IP limits (anonymous runs, tutor IP quota, CAPTCHA endpoint, auth limits, login lockout per IP). Without the header, all clients share the `"unknown"` bucket (a cheap DoS). | `rate-limit.ts` `clientIp` | High if misdeployed |
| L4 | CSRF | `sameOrigin` compares `Origin` against `X-Forwarded-Host` when present, and that header is client-controlled unless the proxy strips it. SameSite=Strict mitigates for authenticated routes; anonymous `/api/run` is the exposed one. | `http.ts` | Low–Medium |
| L5 | Sandbox resources | No cgroup memory/disk limits: `--as` is per process, and `nproc` is "current uid processes + 48/200/264". The sandbox `/tmp` is an unbounded tmpfs (RAM), and `/work` sits on the host tmpdir with only a per-file `fsize` cap. The Go compile has no `--as`. The `MAX_CONCURRENT=4` queue is per process and unbounded. | `runner.ts` limits, `withSlot` | High (availability) |
| L6 | Sandbox escape surface | bwrap with no seccomp filter and no `--disable-userns`. It runs as the web server's uid, so an escape reaches `.env.local`, `.data/db` and the unauthenticated Mongo directly. QEMU (TCG) runs learner-controlled guest code, which adds device-emulation attack surface. | `runner.ts` `sandboxArgs`, `bootKernel` | High (impact) |
| L7 | Tutor | The reference solution is in the system prompt; prompt injection via message, code or `lastOutput` may extract it. `body.code` and `body.lastOutput` aren't type-checked (non-strings reach `buildMessages`). The shared response cache is keyed on user-supplied content, so check for cross-user leakage. Quota refund on failure can be triggered by a cancelled stream. | `api/tutor/route.ts`, `tutor.ts` | Medium |
| L8 | Account model | No email verification at signup: someone can register another person's address, and the signup "account exists" message enumerates emails (CAPTCHA-gated). Username changes free the old name for impersonation on the leaderboard. `changePassword` doesn't require 2FA. | `(auth)/actions.ts`, `settings/actions.ts` | Low–Medium |
| L9 | CSP | `script-src 'unsafe-inline'` in production makes CSP ineffective against XSS. Inventory every `dangerouslySetInnerHTML` (`inline-script.tsx`, `json-ld.tsx`, QR SVG) and every markdown renderer (lessons, tutor replies). | `next.config.ts` | Medium (defence in depth) |
| L10 | Reset token | The token travels in the query string (`/reset-password?token=`), so it lands in proxy/access logs and browser history. | `requestPasswordReset` | Low |

---

## 2. Work packages

Each package lists what to review, what to test, and how to record a finding. Findings go into section 4 with a severity based on exploitability × impact.

### WP1 — Code runner sandbox (highest priority)
**Review:** `src/lib/runner.ts`, `src/lib/kernel/*`, how `exec` builds the bwrap/prlimit command, and the workdir lifecycle (`mkdtemp`, `rm`, `cleanupStaleWorkdirs`).

**Tests:** write one hostile program per language (C, Rust, Go, asm, kernel) and run each through `/api/run` in both `run` and `test` mode:
- Filesystem: list `/`, read `/etc/passwd`, `/proc/1/environ`, `/proc/self/mountinfo`, the host path of the Rust toolchain and `$HOME`. Try writing outside `/work` and `/tmp`, symlink tricks inside `/work` that could affect the host-side `rm -rf` or `readFile(debug.log)`.
- Network: `socket()` to `127.0.0.1:27017`, `127.0.0.1:3000`, an external IP, and abstract unix sockets.
- Namespaces: `unshare(CLONE_NEWUSER)` inside the sandbox, `ptrace` of sibling processes, `/proc/sys` writes.
- Resources: fork bomb, writing until `/tmp` (tmpfs) is exhausted, many 4 MB files in `/work`, threads × address space, a Go compile bomb (generics/const blow-up), a rustc compile bomb (macro recursion), gcc `#include "/dev/zero"` / huge `.incbin` in asm, and output floods at the 64 KB boundary.
- Escape after timeout: `setsid`/double fork to outlive `SIGKILL` of bwrap (check `--die-with-parent` and `--new-session` behaviour), and processes left running after the response.
- Concurrency: 50 parallel anonymous requests. Measure queue growth, memory and event-loop latency.
- QEMU: confirm there is no monitor, disk or network; fuzz-lite the enabled devices (`vga std`, serial, `isa-debug-exit`); check `debug.log` size bounds.

**Decide:** whether to add seccomp (`--seccomp`), `--disable-userns`, cgroup v2 limits via a systemd scope, a dedicated runner uid, a tmpfs size cap, a global queue cap with a 503 response, and the README's recommended separate runner host (Firecracker/gVisor).

### WP2 — Test-result integrity and XP (L1, L2)
**Review:** every result parser in `runner.ts`, `recordAttempt` in `progress.ts`, and `xpFor`/`levelFor`.

**Tests:** proof-of-concept fake passes for each language (constructor `printf("\n@@PASS x\n"); _exit(0);`, Go `init()` + `os.Exit(0)`, Rust writing to `io::stdout()` directly + `process::exit(0)`, kernel `outb 0xE9` + `outl 0xf4`). Check whether one fake pass for a single test name satisfies `allPassed`. Check whether pass/fail is ever compared against the expected test list.

**Directions for fixes:** emit results on a channel learner code can't write to, such as a pipe fd opened by the harness and closed or `dup2`'d away before student code runs, or a per-run random nonce injected at runtime (not compile time) and required on every line. Check the result count against the known list of test names. For the kernel, use a nonce plus the expected test list. Remove the harness source from `/work` before the binary runs.

**Also:** the race in `recordAttempt` (concurrent Test presses), XP after account deletion and re-creation, and `recompute-xp` correctness.

### WP3 — Authentication and session management
**Review:** `auth.ts`, `(auth)/actions.ts`, `settings/actions.ts`, `security.ts`, `totp.ts`, `captcha.ts`, `crypto.ts`, `password*.ts`, `proxy.ts`.

**Checklist:**
- Sessions: token entropy and storage (sha256 of a 32-byte token), cookie flags (`secure` only when `NODE_ENV=production`, so confirm prod runs that way), expiry, `sessionVersion` revocation on password/2FA change, logout of other sessions, and no session-list UI to revoke individual sessions.
- Login: timing equality for unknown vs known emails (dummy hash), the lockout maths (email + IP keys), lockout as a DoS against a victim's email, and CAPTCHA required after 3 failures (can the counter be reset by `clearLoginFailures` from another IP?).
- 2FA: pending-2FA cookie binding, attempt limit, TOTP replay protection (`lastStep`), backup code entropy (8 chars from 31 = ~39.6 bits, fine) and single use, the admin 2FA requirement, and whether an admin can strip their own 2FA via the DB path only.
- Password reset: token single use, expiry, invalidation on password change, the per-email limit, emails sent to unverified addresses (L8), and token in URL (L10).
- `safeNext`: try `/\t/evil.com`, `/%2F%2Fevil.com`, `/\\evil.com` encoded variants, and `javascript:`.
- CAPTCHA: replay protection relies on a `rate_limits` insert. Check the PoW cost (`MAX_NUMBER = 150 000` SHA-256s is cheap for a bot), the signature covering expiry, and the challenge endpoint as a DoS amplifier.
- Crypto: `AUTH_SECRET` length check, no key rotation story for `enc:v1`, AES-GCM with a random IV.
- Server actions: confirm Next 16's Origin check on actions (read `node_modules/next/dist/docs/`), and that every action calls `mustUser`/`requireAdmin` itself rather than relying on `proxy.ts`.

**Tooling:** extend `scripts/e2e-auth.mts` with a `check()` for each new case.

### WP4 — Authorization and data exposure
- Enumerate every route handler and server action, and list for each: auth required, role, rate limit, `sameOrigin`, and input validation. Tabulate the gaps (e.g. `updateProfile` and `clearTutorHistory` have no rate limit; tutor `DELETE` takes any `exerciseId` string, though it's scoped to the caller's own thread).
- IDOR: all queries are keyed on the session user id, so confirm there's no path that takes a user id or progress id from the client.
- `/admin` and `/api/admin/stats`: role + 2FA enforcement, and what `getSiteStats` returns (PII?).
- Public profiles and leaderboard: respect `profilePublic`, return no email/name/code, check the regex in `searchUsers` (input is stripped to `[a-z0-9_-]`, so confirm), and username enumeration.
- `/api/account/export`: completeness (GDPR) and no other user's data. `auth_events` is matched by email, so check whether a previous owner of the same email address would leak their events.
- Account deletion: `tutor_usage`, `events` and `rate_limits` leftovers vs the privacy notice.
- Client bundles and RSC payloads: grep the lesson page's serialized props for `solution`, `harness` and `cases` expected output.

### WP5 — Input handling and injection
- MongoDB operator injection: every `findOne`/`updateOne` built from request JSON. `readJson` returns `any`-shaped objects, so check that `typeof` guards run before use (`body.code` in tutor, `body.stdin`, `exerciseId`).
- Shell: `compileKernel` uses `sh -c` with a fixed string; confirm no user input is ever interpolated. Compiler flags are fixed.
- Markdown/XSS: lesson markdown (trusted, but reviewed), tutor replies (model output, so untrusted), `react-markdown` URL handling (`javascript:` links), no `rehype-raw`, and the QR SVG source.
- Compiler output and program output rendered in the workspace: confirm they're rendered as text, not HTML/ANSI.
- Headers: `Content-Disposition` filename, and `X-Tutor-*` values.

### WP6 — AI tutor
- Prompt injection to extract `<reference_solution>` (try code comments, fake `</current_code>` closers, and output containing instructions). Decide whether the solution should be in the prompt at all, or only a rubric.
- Response cache: can user A's crafted first message plant a reply that user B receives? The key includes the lesson context, message, code and output, so assess how likely collisions are on common starters.
- Quota: per-user, per-IP and global limits, the IP limit's dependence on L3, the refund path (`$inc: -1` triggered by aborting before the first token?), and the history and size caps.
- Ollama mode: `OLLAMA_HOST` must stay local; check for SSRF via any configurable value.

### WP7 — HTTP, browser and transport hardening
- CSP without `'unsafe-inline'` (nonces via Next 16, after checking the docs), HSTS preload readiness, COOP/CORP, and `Permissions-Policy`.
- `Cache-Control` on authenticated pages (not just `/api`), and cache of RSC payloads behind a CDN.
- Cookies: `bm_session`, `bm_2fa_pending`, consent cookie, `bm:theme` localStorage; check they match `content/legal/cookies.md`.
- Error pages: no stack traces in production (`error.tsx`, `global-error.tsx`, `[run] runner failure` logging).

### WP8 — Deployment and configuration
- Reverse proxy: must overwrite `X-Forwarded-For` and drop client `X-Forwarded-Host` (L3, L4); set `TRUSTED_PROXY_HOPS` correctly.
- MongoDB: enable auth, a least-privilege app user, bind address, backups and encryption at rest for `.data/db`.
- Secrets: `.env.local` is gitignored (confirmed). Check git history for any committed secret (`git log -p -S AUTH_SECRET`, `gitleaks`), file permissions, and the `ANTHROPIC_API_KEY` scope and spend limit.
- Process: run `next start` as a dedicated unprivileged user, under a systemd unit with `ProtectSystem`, `PrivateTmp` and `MemoryMax`.
- SMTP: SPF/DKIM/DMARC, TLS, and no credentials in logs.
- Logging and monitoring: what `console.warn` emits (emails in logs), log retention vs the privacy notice, and alerting on `rate_limited` / `login_locked` spikes.

### WP9 — Dependencies and supply chain
- `npm audit`, `npm ls` for duplicate/old versions, and lockfile integrity.
- Review the high-risk packages: `next`, `react-markdown` / `remark-gfm`, `qrcode`, `nodemailer`, `mongodb`, `@anthropic-ai/sdk`.
- Toolchain versions in the sandbox (gcc, rustc, go, qemu, bubblewrap, kernel) against current CVEs, since these are the escape surface.

### WP10 — Privacy and legal consistency
- Every stored field and collection vs `content/legal/privacy.md` (retention TTLs in `db.ts`: events 180 d, auth_events 90 d, tutor_cache 30 d, tutor_usage 180 d).
- The analytics visitor hash (daily HMAC salt + IP + UA) is pseudonymous rather than anonymous, so the notice should say so.
- Data sent to Anthropic (code, questions), stated in the notice and gated appropriately.
- HIBP k-anonymity call (first 5 SHA-1 chars), which is fine; disclose it.

---

## 3. Method and schedule

| Phase | Work | Output |
|---|---|---|
| 1. Setup (½ day) | Production-like instance: `npm run build && npx next start -p 3100` behind nginx, Mongo with auth, two test accounts plus an admin with 2FA. Snapshot the host. | Test environment |
| 2. Confirm leads (1 day) | Proof-of-concept for L1–L10 | Confirmed/rejected list |
| 3. Sandbox deep dive (2 days) | WP1, WP2 | Findings + hardening proposal |
| 4. App review (2 days) | WP3–WP7, manual code read of every file under `src/app/api`, `src/app/**/actions.ts`, `src/lib` | Findings |
| 5. Config and deps (½ day) | WP8–WP10, `npm audit`, secret scan | Findings |
| 6. Report (½ day) | Write up section 4, then prioritise fixes | Final report |

Tools: Burp/ZAP for request tampering, `curl` scripts, hostile programs kept in `scripts/security/` (not in `content/`), `gitleaks`, `npm audit`, `ss`/`ps` to watch sandbox processes, and `stress-ng`-style programs for resource tests. Every confirmed issue gets a regression `check()` in `scripts/e2e-auth.mts` or a new `scripts/e2e-sandbox.mts`.

Safety: sandbox tests run only on the audit instance, never on a shared or production host. Fork-bomb and RAM-fill tests run inside a VM or a cgroup with `MemoryMax`.

---

## 4. Findings log (fill in during the audit)

| ID | Title | Severity | Status | Evidence | Fix | Regression test |
|---|---|---|---|---|---|---|
| | | | | | | |

Severity scale: **Critical** (host compromise, mass account takeover) · **High** (single account takeover, sandbox DoS of the site, XP integrity broken at scale) · **Medium** (data leak of limited scope, hidden-test leakage, CSP gaps) · **Low** (hardening, information disclosure without direct impact).

---

## 5. Exit criteria

- Every lead in section 1 is confirmed with a PoC or rejected with a reason.
- Every API route and server action is in the WP4 authorization table.
- No open Critical/High findings, or each has an accepted, dated mitigation.
- New regression checks pass alongside the existing 45 in `scripts/e2e-auth.mts`, plus `npm run verify:content`, `npx tsc --noEmit`, `npm run lint` and `npm run build`.
- README "Before going to production" is updated with the deployment requirements from WP8.
