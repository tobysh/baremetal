# Handoff — Baremetal

Status as of 29 October 2026. Read this, then `CLAUDE.md` (architecture, commands, theme rules), `README.md`, `GETTING_STARTED.md` and `docs/local-setup.md`.

Recovery update, 29 October 2026: interrupted Claude content and security work has been continued in the main working tree. There are now **26 courses, 276 lessons, 3305 lesson XP, 23 reviews and 4 challenges**. All courses have complete file sets; the full content verifier passed twice, including the final repeat after parser review. The new security path has 38 lessons, with deterministic synthetic packet/evidence generators. Concurrency and shell tracks received repeated checks; timing-dependent harness assumptions were replaced with synchronization.

The saved result-integrity and deployment-guard changes are integrated, along with quick fixes, account hardening and runner resource support. Production build, auth e2e, API flood/quota/hint checks, browser hint/mobile/auth smoke checks, XP race fixtures, forged-result fixtures, cancellation-cache regression and renderer checks pass. TypeScript passes; lint has zero errors and one pre-existing unused-variable warning in `scripts/security/integrity/poc-fakepass.mts`. Production dependency audit reports zero advisories. No commits or merges were made by this recovery pass; original security worktrees/branches remain available.

**Deployment work still required:** configure SMTP and the periodic encrypted-email queue worker; configure/test a delegated cgroup v2 subtree (`RUNNER_CGROUP_ROOT`) on the real host; retain aggregate service limits and uid isolation. Per-job cgroup enforcement could not be exercised because this workspace exposes cgroups read-only. Strict nonce CSP remains deferred because the installed Next.js guide says it is incompatible with the app's partial prerendered shells. Full transcript forgery by code sharing a process with its harness remains a documented limitation; exact transcripts, nonce randomization and XP caps reduce abuse but do not create an out-of-process verdict. See `docs/deployment-hardening.md` and `docs/security-remediation-plan.md` for current details.

**Operational notes:** new accounts verify email and choose their final password through the verification link; legacy accounts without a verification flag remain usable. Backup codes use purpose-separated HMACs and longer codes, with single-use legacy support. XP pace and daily admission use an atomic per-user MongoDB update; interrupted first-pass decisions set `xpPending` and are listed by `npm run flagged-accounts`. Run XP recomputation only with the site stopped; it reconciles recorded pending decisions and refuses to assume an award when no decision exists.

The sections below retain earlier feature context; this recovery update supersedes their old counts and security-status notes.

## What exists

- Next.js 16.4 app (App Router, Cache Components, Turbopack), Tailwind v4, local MongoDB (`MONGODB_DB=baremetal`).
- **26 courses, 276 lessons, 3305 XP** (lessons only; challenges add half XP each) in `content/`, in six learning paths: Start here (Programming from Zero, 15 lessons), C (3 courses), Rust (4), Go (2: Foundations, Go in Practice) and Machine level (x86-64 assembly, and Build an Operating System). 23 lessons are `kind: review` consolidation lessons; lessons can carry `tags:`.
- Course order lives in each `content/<track>/track.json` (`path`, `order`, `prerequisite`); the paths are defined in `src/lib/curriculum/paths.ts`. `/learn` groups by path and the home page shows the path name on each card.
- Four languages plus a kernel mode, all run in a bubblewrap sandbox (no network, CPU/memory/process limits): C (gcc, UBSan), Rust (rustc), Go (`go test`), x86-64 assembly (GNU `as`, Intel syntax, tested by hidden C) and `kernel` (the osdev track boots the learner's freestanding C under QEMU). Details in `CLAUDE.md`.
- `npm run verify:content` checks every lesson (solution passes, starter fails). Needs gcc, rustc, go, bubblewrap and qemu-system-x86_64.
- AI tutor (`src/lib/tutor.ts`, `/api/tutor`): Claude Haiku 5.5 by default, or Ollama (`TUTOR_PROVIDER=ollama`, CPU inference is slow without `ollama-cuda`), response cache, quotas per user/IP/global, pre-written failure explanations (`src/lib/diagnostics.ts`). Needs `ANTHROPIC_API_KEY` in `.env.local` or Ollama installed; check that it is set before relying on it.
- Auth: scrypt, hashed sessions in httpOnly SameSite=Strict cookies, TOTP 2FA + backup codes (required for admins), lockout with backoff, PoW CAPTCHA after 3 failures, HIBP + top-10k password checks, IP limits. `npm run test:e2e -- http://localhost:3100` passes (expanded auth/security suite, against `next start -p 3100`).
- Gamification (design and deviations in `docs/gamification-plan.md`; code map in `CLAUDE.md`): XP (5/10/20/40 by difficulty, ledger in `xp_events`), levels (`LEVEL_FLOORS` table in gamification.ts: 8, 24, 60, 120, 210, 330, 500, 720, 1000 at the level-10 cap, XP keeps counting), medieval titles, first-pass XP in `recordAttempt`, `/leaderboard` and `/u/[username]` public profiles (noindex, opt-out in settings). The header level chip refreshes after a first pass (`router.refresh()` in the workspace). Also built: hints recorded server-side, three seals per lesson (Passed, Unaided, Challenge), 4 optional challenges (`<id>#challenge` progress docs, half XP, locked until the lesson is passed; in c-memory/08, asm-first-steps/04, rust-foundations/04, go-foundations/03), a 20-entry Codex (no XP), weekly rhythm derived from `passedAt` (goal and time zone in settings, banked rest weeks), spaced reviews at 3/7/21/60 days (no XP), dashboard course map, stats and daily pick, level-up and course-complete dialogs, and a profile showcase. Export, deletion and the privacy page cover the new collections.
- Theme: dark default, light via OS or `data-theme`, tokens only (see `CLAUDE.md`). The sitewide look comes from the dashboard redesign (`docs/design/`): `.panel` surfaces, Archivo wide display headings (`.display`, `.display-sm`), amber only for progress and action, underlined active nav. Text contrast measured AA in both themes. Form-field borders use `--field` (3:1); `--border`/`--line` are decorative dividers only.
- Editor: failed compiles, panics and UBSan reports tint the offending lines (`src/lib/error-lines.ts`, parsed client-side from the run output). The active-line highlight is hidden while text is selected, otherwise it covers the last selected line.
- Review lessons and "last lesson" references link to the lessons they build on (`/learn/<track>/<slug>`). Add links the same way in new lessons.
- GDPR/SEO: legal pages from `content/legal/*.md` (privacy covers public profile data, cookies covers `bm:theme`), consent banner, first-party analytics, export/delete in settings, robots, sitemap, llms.txt, og:images, JSON-LD, canonical, skeletons, 404.

## Known gaps

- **`/u/<valid-name>` for a user that doesn't exist returns HTTP 200** with the not-found page. Malformed names return a real 404. A true 404 needs the existence check before streaming starts, which would make the page blocking; left as is because the pages are `noindex`. `generateStaticParams` in that page returns one sample name only to satisfy Cache Components at build time.
- **375px header** was checked only as static markup in a narrow frame (it wraps to two rows, nothing clipped, no horizontal scroll), not on a real phone-width viewport.
- **Gamification:** phases A to D4 are built. D5 (community features: `communityReady()` helper, seasons, rank gating) is not, by request; the user-count threshold is an open decision in the plan. Deviations from the plan (challenges as `#challenge` progress docs, derived rhythm, no `bestMs`) are in the plan's status block.
- Intermittent rustc/LLVM linker address-space aborts recurred during recovery; limiting lld to one worker fixed the repeat runs without relaxing execution limits.
- Turbopack warns about dynamic file access at `src/lib/runner.ts:211`. Harmless so far.
- Not production-ready: needs a reverse proxy setting `X-Forwarded-For` (`TRUSTED_PROXY_HOPS`), SMTP, Mongo auth and legal review. See README "Before going to production".

## Suggested next steps

In rough priority order:

1. **Run `npm run recompute-xp` and then `npm run backfill-achievements` once on any existing database** (creates the XP ledger and review schedule, awards earned achievements). Level thresholds changed, so existing learners' levels will drop; their XP is unchanged.
2. **Click through the new UI in a real browser** (Playwright with headless Chromium works; on this machine set `CHROME=` to the `chromium_headless_shell` under the Playwright browsers dir): lesson, challenge and review tabs, both celebration dialogs (focus trap, Escape, reduced motion), toasts, at 1440px and 390px; then a real phone.
3. **Production gaps:** footer "operator name to be confirmed", legal review (the privacy text now covers hints, tutor use before a pass, the XP ledger, achievements, review schedule, rhythm goal and time zone, and the extra public-profile fields), SMTP, Mongo auth, `X-Forwarded-For` from a reverse proxy (`TRUSTED_PROXY_HOPS`). See README "Before going to production".
4. **Tutor token cost** (the only per-use cost): send less lesson context, shorter replies, tighter code and output caps, a dollar ceiling per user and per day, a normalised cache key.
5. **Test-result integrity (`docs/security-audit.md` L1) is mitigated, not fixed.** Results carry a per-run nonce (C/asm/kernel), submissions containing harness protocol text are rejected, and first-pass XP is paced (6 per 10 minutes). A determined learner can still forge by reading the nonce out of their own process. The root fix is an out-of-process verdict (driver prints raw values, server diffs against a reference transcript), which means rewriting the lesson tests. The sandbox is also hardened (seccomp, no user namespaces, read-only work dir at run time, capped tmpfs, bounded queue); cgroup limits and uid separation are deployment tasks in `docs/deployment-hardening.md`.
6. **Content ideas:** more challenges (only 4 exist; add the file set `challenge.md`, `challenge-starter.*`, `challenge-solution.*`, `challenge-tests.*` or `challenge-cases.json`, and `verify:content` checks them), arrays/strings/structs before C memory, more Go (context, `net/http`, testing), an intermediate Rust course (traits, lifetimes, errors, concurrency).
7. **Admin view of where learners get stuck** (lowest pass rates, most hints, most tutor use), and a screen-reader pass over the new dialogs and toasts.
8. Decide the community threshold before building D5.
9. Docs worth reading: `docs/gamification-plan.md`, `docs/security-audit.md` (open audit plan, PoCs in `scripts/security/`, currently untracked), `docs/design/` (dashboard handoff and reference).

## Gotchas

- Node via nvm; Rust in `~/.cargo/bin`; MongoDB binaries in `~/.local/bin` (`npm run db`).
- After adding routes run `npx next typegen`, or `PageProps<"/route">` won't typecheck.
- Next 16 here differs from training data: read `node_modules/next/dist/docs/` before changing framework-level code (`AGENTS.md`).
- Pages that read the DB during render need `await connection()` inside `<Suspense>`.
- Lesson ids (`track/slug`) are the progress keys. Renumbering folder prefixes is safe; changing a slug or `id:` loses learners' progress. After changing a lesson's difficulty run `npm run recompute-xp`.
- Adding a language touches the runner, curriculum types, editor grammar, diagnostics and the tutor prompt.
- Rust here is installed system-wide (`/usr/bin/rustc`), not via rustup. The runner now detects `/usr` automatically; `RUST_TOOLCHAIN_DIR` still overrides detection.
- Don't `pkill -f` with a pattern that also appears in your own command line (it kills the shell). Kill by PID.
- `verify:content -- osdev` reports a bare "triple fault" for every lesson when `qemu-system-x86_64` is missing; install QEMU before trusting a failure.
- Commits in this repo carry no co-author or "Generated with" lines.
- Changing `LEVEL_FLOORS`, a lesson's difficulty or challenge XP needs `npm run recompute-xp`; it rebuilds the ledger per user (delete and reinsert).
- Scratch scripts that import `mongodb` or repo code only resolve from inside `scripts/`; delete them afterwards.
