# Architecture

How Baremetal fits together, where each responsibility lives, and why. For the threat model see [security.md](security.md); for running it in production see [deployment-hardening.md](deployment-hardening.md).

## The shape of the system

```
                         ┌────────────────────────── browser ──────────────────────────┐
                         │ lesson page · CodeMirror editor · tutor chat · xterm shell  │
                         └───────┬───────────────────────────────────────────┬─────────┘
                                 │ HTTPS (via reverse proxy)                 │ SSE + POST
                   ┌─────────────▼─────────────┐                             │
                   │  web app  (Next.js 16)    │                             │
                   │  pages · route handlers · │                             │
                   │  auth · XP · tutor        │                             │
                   └──┬──────────┬──────────┬──┘                             │
        MongoDB       │          │ Unix     │ Unix socket                    │
   (users, progress,  │          │ socket   │                       ┌────────▼────────┐
    ledger, caches)   │   ┌──────▼──────┐   └──────────────────────►│  shell broker   │
                      │   │ runner      │                           │  (unprivileged) │
                      │   │ service     │                           └────────┬────────┘
                      │   │ bubblewrap: │                                    │ Unix socket
                      │   │ gcc rustc go│                           ┌────────▼────────┐
                      │   │ as · qemu   │                           │ launcher (root) │
                      │   └─────────────┘                           │ jailer+Firecracker VM per session
                      │                                             └─────────────────┘
            Anthropic / Ollama  (tutor)
```

Four independently deployable pieces:

| Piece | Entry point | Privilege | Job |
|---|---|---|---|
| Web app | `next start` | web uid; holds `AUTH_SECRET`, DB credentials, API keys | Pages, API routes, accounts, progress, tutor |
| Runner service | `scripts/runner-service.mts` (`npm run runner`) | its own uid; no web secrets | Compile and run learner code in bubblewrap; owns admission, queueing and cancellation |
| Shell broker | `scripts/shell-broker.mts` | unprivileged; refuses to run as root | Interactive shell sessions, limits, framing |
| Shell launcher | `scripts/shell-launcher.mts` | root, reduced capabilities | The only part that starts and kills Firecracker microVMs |

The shell is optional and not yet verified on a real host; see [interactive-shell-isolation.md](interactive-shell-isolation.md). In development you run only MongoDB and `next dev`: with `RUNNER_SOCKET` unset, `src/lib/runner-client.ts` calls the execution engine in-process (under your own uid). Production refuses to start without `RUNNER_SOCKET` (`src/instrumentation.ts`) and never falls back to in-process execution.

## Source layout

```
content/                 the curriculum (one folder per track) and content/legal/*.md
src/app/                 App Router pages and route handlers
src/components/          React components; workspace/ is the lesson editor UI
src/lib/                 server logic (see the map below)
src/lib/curriculum/      content loader, types, learning paths
src/lib/kernel/          hidden boot stub, runtime and linker script for kernel lessons
src/lib/shell/           broker, launcher, framing and web side of the interactive shell
src/proxy.ts             Next 16 middleware (per-request CSP nonce, optimistic redirect for signed-out visitors)
src/instrumentation.ts   production boot checks (refuses to start if misconfigured)
scripts/                 CLIs, verifiers, regression suites (see CONTRIBUTING.md)
deploy/                  systemd units, nftables, udev and tmpfiles files
docs/                    this documentation
```

### `src/lib` map

| Area | Files | Notes |
|---|---|---|
| Curriculum | `curriculum/index.ts`, `types.ts`, `paths.ts`, `learning.ts` | Loads and validates `content/`; `learning.ts` computes recommendations and prerequisite checks |
| Execution | `runner-client.ts`, `runner-protocol.ts`, `runner-engine.ts`, `runner-resources.ts`, `runner-types.ts`, `runner.ts` | Client used by the web app, wire protocol (zod-validated), the sandbox engine, per-job cgroups. `runner.ts` is a thin re-export of the engine for scripts |
| Accounts | `auth.ts`, `crypto.ts`, `totp.ts`, `captcha.ts`, `password*.ts`, `email.ts`, `redirects.ts`, `consent.ts` | scrypt, hashed session tokens, TOTP, lockouts, PoW CAPTCHA, encrypted email queue |
| HTTP hardening | `http.ts`, `rate-limit.ts`, `security.ts` | `sameOrigin`, bounded `readJson`, per-key rate limits, trusted-proxy IP handling |
| Progress and rewards | `progress.ts`, `progress-constants.ts`, `gamification.ts`, `rewards*.ts`, `achievements.ts`, `rhythm.ts`, `daily.ts` | See [gamification.md](gamification.md) |
| Tutor | `tutor.ts`, `tutor-budget.ts`, `tutor-cache.ts`, `tutor-limits.ts`, `tutor-normalize.ts`, `tutor-popular.ts` | Prompting, `SolutionGuard`, spend reservations, shared answer cache |
| Feedback | `diagnostics.ts`, `error-lines.ts`, `stuck.ts`, `learning-report.ts`, `learning-cohorts.ts` | Regex-matched failure explanations; admin "where learners get stuck" and cohort reports |
| Community | `community.ts`, `stats.ts` | Leaderboard and public profiles |
| Misc | `db.ts`, `site.ts`, `legal.ts` | Mongo connection and indexes; site constants and legal details |

## Content pipeline

Everything a learner sees comes from files under `content/<track>/`. `src/lib/curriculum/index.ts` reads them at startup (`process.cwd()/content`) into `Track` → `Lesson` → `Exercise` objects. Authoring details are in [content-authoring.md](content-authoring.md); the architectural points are:

- A track's `language` (`c`, `rust`, `go`, `asm`, `kernel`) fixes the file extensions, the editor file name (`LANGUAGE_FILE`) and the label (`LANGUAGE_LABEL`) in `types.ts`.
- An exercise is either `mode: stdout` (compare program output against `cases.json`) or `mode: harness` (a hidden `tests.*` file calls the learner's code).
- **Hidden material never reaches the browser.** Pages use `toPublicExercise`, which carries `id`, `language`, `task`, `starter`, `mode` and `hintCount` only. Solutions and tests stay server-side; hints are fetched one at a time through `/api/hint`, which records their use.
- **Lesson ids (`track/slug`) are progress keys.** `content/.lesson-ids.json` is a snapshot, and `npm run check:ids` fails if a published id disappears. Renumbering folder prefixes is safe; changing a slug or `id:` loses learner progress.
- **Drafts.** `"draft": true` in `track.json` (whole course) or `draft: true` in a lesson's frontmatter keeps unfinished work on disk but out of the catalogue, search, recommendations, static routes and exercise lookup. Missing files in *published* content are hard errors, by design.
- **Paths.** Tracks group into learning paths (`start`, `c`, `rust`, `go`, `low`, `security`) via `path` and `order` in `track.json`; `paths.ts` defines the list. Optional `prerequisite` links drive recommendations and are validated at verification time.
- **Challenges.** A lesson can carry an optional harder variant (`challenge.md`, `challenge-*` files). Its exercise id is `<lesson id>#challenge` and it is stored as an ordinary progress document.

## Running learner code

### Request path (`POST /api/run`)

1. `sameOrigin` rejects cross-site requests. In production only the host of `NEXT_PUBLIC_SITE_URL` is accepted; `X-Forwarded-Host` is never read.
2. The user is resolved from the `__Host-bm_session` cookie (or the visitor is anonymous) and a rate limit is applied *before* the body is parsed: 120 per 10 min per account, `RUN_LIMIT_ANON_IP` (default 30) per hashed IP for anonymous visitors.
3. `readJson` reads a bounded body; code is capped at 64 KB. `findExercise` resolves the id (including `#challenge` ids). Challenges require the base lesson to be passed first.
4. `executeRun` / `executeTest` in `runner-client.ts` send a versioned, size-capped request over the runner's Unix socket. Admission keys (`user:<id>` for signed-in learners, `ip:<ip>` for anonymous) identify the caller for in-flight limits.
5. For a test run by a signed-in learner, `recordAttempt` stores the code, grants first-pass XP and updates the ledger; `evaluateAchievements` runs on a first pass. Review runs move only the review schedule.

### Inside the runner (`runner-engine.ts`)

- Two slot pools: compile (`RUNNER_COMPILE_SLOTS`, default 2) and run (`RUNNER_RUN_SLOTS`, default 3), each with a queue of up to 24 waiters and a 30 s wait limit; beyond that callers get 503 (`RunnerBusyError`). A job holds a compile slot only while its compiler runs. Free slots go to the waiting owner served least recently, not first come first served; a signed-in account holds at most one compile slot at a time and each compile (including `set`-case recompiles) is charged to `RUNNER_COMPILE_BUDGET` (default 60 per minute). Aborting the HTTP request cancels the job.
- Every compiler and every learner program runs under **bubblewrap**: no network, read-only system, `--disable-userns`, an x86-64 seccomp denylist, rlimits, a 5 s timeout, output caps. Compilation uses size-capped private tmpfs with read-only sources; only the executable returns, over a capped pipe. With `RUNNER_CGROUP_ROOT` each job also gets its own cgroup-v2 limits.
- Per language:
  - **C**: `gcc` builds the learner's `main.c` with the hidden harness appended. A lesson with a `driver.c` instead builds `main.c` alone to an object and links it with a separately compiled driver (see the test protocol below).
  - **Rust**: `rustc` with gcc as linker; harness tests are `#[test]` functions appended to `main.rs` and run via `--test`.
  - **Go**: `main.go` plus the hidden `tests.go` (as `bm_test.go`) in a throwaway module (CGO off, `GOPROXY=off`), built with `go test -c` and run with `-test.v`. The std library compiles cold in about 3 s; a prewarmed `GO_CACHE_SEED` overlay cuts that to about 0.3 s.
  - **Assembly**: `starter.s` is assembled alone in its own directory and linked with a hidden C `tests.c` that has its own `main`.
  - **Kernel** (the `osdev` tracks): the learner's `kernel.c` is linked with `src/lib/kernel/{boot.S,runtime.c,link.ld}` and booted with `qemu-system-x86_64 -kernel` (TCG, no disk or network) inside bubblewrap. `tests.c` is appended and defines `kmain()` (the learner's own `kmain` is renamed with `-Dkmain`). Results travel over the QEMU debug console (port 0xE9) so they never mix with the learner's serial output on COM1; exit is `isa-debug-exit`, a CPU exception prints `@@EXC`, a bare reset is reported as a triple fault.
- **Test protocol.** Harnesses print `@@RUN/@@PASS/@@FAIL` lines through a shared `CHECK` macro (or the Go/Rust equivalents). `testExercise` accepts a result only if the exact ordered list of test names matches what the reference solution produces, the run exited normally, and (C/asm) the per-run nonce in `BM_NONCE` matches. Harness diagnostics are redacted and a source scan rejects attempts to read the hidden harness (`#include`, `include_str!`, `.incbin`, `#embed`, …). For harness lessons the verdict is still produced in the learner's own process, so a determined learner can forge a pass; XP pacing bounds the damage. C lessons with a `driver.c` use an out-of-process verdict instead (`src/lib/verdict.ts`): the driver records inputs and results to fd 3, the same driver linked with the reference solution runs with the same random seed in another sandbox, and the server compares the two transcripts. The honest account is in [security.md](security.md).

### Failure feedback

`src/lib/diagnostics.ts` holds pre-written, regex-matched explanations per language with a suggested next debugging step. They work with no AI call. Raw output and diffs are always retained alongside them.

## The tutor

`src/lib/tutor.ts` talks to Anthropic (default; `TUTOR_MODEL`, default `claude-haiku-5-5`) or a local Ollama model (`TUTOR_PROVIDER=ollama`). The app works with no provider configured.

- The system prompt coaches rather than answers. **The model never receives the reference solution.** Learner text is tag-escaped, and replies stream through `SolutionGuard`, which withholds the last 24 tokens until every window has been compared with the solution; on a match the rest of the reply is replaced with a fallback sentence and not cached.
- Cost control: atomic per-user and global daily dollar reservations in `tutor_spend` (`tutor-budget.ts`), hourly request quotas (`tutor-limits.ts`), and a privacy-checked shared answer cache with longer retention for popular answers (`tutor_cache`). Newcomers draw from a pool that reserves capacity for established users.
- Conversations are stored per user and lesson (`tutor_threads`). Using the tutor before a pass affects the Unaided seal via a sticky progress flag.

## Accounts and sessions

`src/lib/auth.ts`: scrypt password hashes; random session tokens stored only as hashes (`sessions`), delivered in an httpOnly, SameSite=Strict `__Host-bm_session` cookie (`bm_session` only with `BM_INSECURE_COOKIES=1`); optional TOTP with replay protection and backup codes; atomic attempt reservation and pair-scoped lockouts; a proof-of-work CAPTCHA; HIBP and common-password checks (`data/common-passwords.txt`); mandatory email verification for new accounts through an encrypted, retried queue (`email_queue`, drained by `scripts/deliver-email.mts`). **Admin panel** (`/admin`): overview with system health and tutor cost, users (search, create, reset-link, resend verification, lock, ban, sign out everywhere, delete), security log, and the stuck report. Queries live in `src/lib/admin.ts`, mutations in `src/app/admin/actions.ts`, which re-check `requireAdmin` on every call, rate limit per admin, refuse to touch admin accounts or the caller's own, and write `admin_audit`. Granting or removing the admin role stays in `scripts/make-admin.mts`. A lock or ban is `users.suspension`; `isSuspended` in `auth.ts` makes `loadUser` return null, so existing sessions stop working at once, and login and the 2FA step refuse. The admin never sees or sets a password: reset sends the owner a link, and admin-created accounts get a verification link where the owner chooses a password and accepts the Terms (`termsVersion: "pending"` until then). Account removal is shared with self-service deletion in `src/lib/account-ops.ts`. `src/proxy.ts` sets the per-request CSP and redirects signed-out visitors away from `/dashboard`, `/settings` and `/admin` as an optimisation; the real checks (`requireUser`, `requireAdmin`, 2FA for admin) run on the server in every page and route.

Client IPs come from `X-Forwarded-For` as written by *your* proxy; `TRUSTED_PROXY_HOPS` says how many hops to trust. See "Proxy contract" in [deployment-hardening.md](deployment-hardening.md).

## Progress, XP and rewards

`recordAttempt` (`src/lib/progress.ts`) is the single place XP is granted: once per exercise, on first pass, through one atomic pipeline update guarded by a per-user pace limit, a rolling 24 h cap and a minimum solve time. Suspect passes are saved without XP (`xpWithheld`) and can be reviewed with `npm run flagged-accounts`. Each grant is a row in `xp_events` (unique on `userId, source, ref`), so XP is rebuildable from stored data with `npm run recompute-xp -- --confirm-backup`. A pass interrupted by a crash (`xpPending`) is settled by `reconcilePendingXp`, started from `instrumentation.ts`. Production refuses to start without `MONGODB_URI`. Levels, seals, challenges, the Codex, weekly rhythm and spaced reviews are described in [gamification.md](gamification.md).

## Data model

MongoDB, database `MONGODB_DB` (default `baremetal`). Indexes are created on first connection in `src/lib/db.ts`.

| Collection | Purpose | Retention |
|---|---|---|
| `users` | Account, hashed password, TOTP, XP, profile and rhythm settings | Until deletion |
| `sessions` | Hashed session tokens | TTL on `expiresAt` |
| `progress` | One doc per `(userId, exerciseId)`: status, code, hints used, snapshots at first pass. Challenges use `<id>#challenge` | Until deletion |
| `xp_events` | XP ledger | Until deletion |
| `achievements`, `reviews` | Codex unlocks; spaced-review schedule | Until deletion |
| `review_events` | One row per schedule change, skip or retry (source, correct, rating, days late, intervals); index `(userId, at)` | Until deletion |
| `tutor_threads`, `tutor_usage`, `tutor_spend` | Conversations; per-question usage records (feed the admin stuck report); daily dollar reservations | Spend rows expire after 60 days. `tutor_usage` rows carry `owner` (HMAC of the user id), are exported and deleted with the account, and expire after 180 days; rows from before `owner` existed have none and only age out |
| `tutor_cache` | Shared, privacy-checked answers | TTL |
| `rate_limits`, `login_failures`, `pending_2fa` | Counters and short-lived auth state | TTL |
| `email_queue`, `email_verifications`, `password_resets` | Account email flow | TTL (queue: 7 days) |
| `events`, `auth_events` | Consent-based analytics; auth audit | 180 and 90 days |
| `admin_audit` | One row per admin-panel action: admin id, action, target account id, optional note | 1 year |

Every collection that holds personal data is covered by account export (`/api/account/export`), account deletion, and `content/legal/privacy.md`. `npm run sweep-orphans` reports rows whose user no longer exists (dry run; `-- --apply` deletes them).

## Web app conventions (Next.js 16)

This repo uses Cache Components, partial prerendering and Turbopack, and **Next 16 differs from what most tooling and training data assume**: read the relevant guide in `node_modules/next/dist/docs/` before changing framework-level behaviour (see `AGENTS.md`).

- Anything that reads cookies, the database or dynamic params goes inside `<Suspense>` with a skeleton fallback. Pages that read MongoDB during render must `await connection()` from `next/server` first, otherwise the driver's `new Date()` breaks prerendering.
- After adding routes, run `npx next typegen` so `PageProps<"/route">` types resolve.
- Heavy client code (editor, password checker, command palette, terminal) is lazy-loaded.
- Every page sets its own `title`, `description` and `alternates.canonical`, and renders exactly one `<h1>`.
- The CSP (`src/lib/csp.ts`) has no `script-src 'unsafe-inline'`: the proxy adds a per-request nonce for the scripts streamed by the resume and the hashes of the few fixed inline scripts in the prerendered shells (see `docs/security.md`). The one application inline script is the static theme bootstrap.

## Phones and the catalogue

- **Lesson layout** (`workspace/lesson-layout.tsx`): below 1024px the lesson text and the code workspace are two tabs ("Read lesson", "Code & feedback"), and a horizontal swipe on the text or panels switches them (touches starting in the editor, code blocks or form fields are ignored). Desktop keeps the resizable split pane.
- **Symbol bar** (`workspace/symbol-bar.tsx`, `snippets.ts`): on coarse-pointer devices, while the editor is focused and no hardware keyboard has been detected (a modifier chord or navigation key; in memory only, nothing stored), a toolbar is docked above the on-screen keyboard using `visualViewport`. It has snippet chips per language (CodeMirror snippets with tab stops; the Tab key jumps to the next stop), symbols, Tab, undo/redo and a drag pad that moves the caret. The editor also turns off autocapitalise/autocorrect/spellcheck.
- **Help sheet** (`workspace/help-sheet.tsx`): a phone-only floating "Help" button opens a bottom sheet with the authored hints and one-tap tutor prompts.
- **Lesson-type filter**: `/learn` and `/learn/[track]` take `?type=` (validated by `parseTypeFilter`). Both pages always render the full catalogue; a small Suspense-wrapped part reads the param and emits CSS that hides non-matching courses or lessons, so the pages stay cacheable and indexable. Types with no lessons get no chip, and canonicals ignore filter params. Each syllabus row shows its types as icon plus text.

## Theme

Colours come from tokens in `src/app/globals.css` only (`bg`, `surface`, `surface-2`, `border`, `text`, `muted`, `accent`, `ok`, `err`, `--code-*`, difficulty colours), each with dark and light values. `npm run check:theme` rejects hard-coded hex colours and left accent stripes in components. Fonts are IBM Plex Sans and Mono. See [CONTRIBUTING.md](../CONTRIBUTING.md#ui-rules).

## The interactive shell (optional)

A separate subsystem that gives signed-in, verified learners a terminal in a throwaway Firecracker microVM. The web side (`src/app/shell`, `src/app/api/shell/**`, `src/lib/shell/web*.ts`) talks to the broker over a Unix socket using Server-Sent Events for output and POSTs for input. The broker (`broker.ts`) enforces sessions, caps, rate limits and framing; it asks the root launcher (`launcher.ts`) to create and destroy VMs. The guest is assumed hostile and holds no secrets. Design: [interactive-shell-isolation.md](interactive-shell-isolation.md). Host install: [shell-deployment.md](shell-deployment.md).
