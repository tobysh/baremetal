# Handoff — Baremetal

Status as of 27 October 2026. Read this, then `CLAUDE.md` (architecture, commands, theme rules), `README.md`, `GETTING_STARTED.md` and `docs/local-setup.md`.

The last full pass was green: `verify:content` for the asm and osdev tracks (with QEMU installed), `tsc`, lint, `next build` and the e2e suite. The rest of `verify:content` was last run before the UI overhaul and the lesson-link edits, which touched markdown only.

## What exists

- Next.js 16.4 app (App Router, Cache Components, Turbopack), Tailwind v4, local MongoDB (`MONGODB_DB=baremetal`).
- **11 courses, 97 lessons, 1160 XP** in `content/`, in five learning paths: Start here (Programming from Zero), C (3 courses), Rust (4), Go (1) and Machine level (x86-64 assembly, and Build an Operating System). 9 lessons are `kind: review` consolidation lessons; lessons can carry `tags:`.
- Course order lives in each `content/<track>/track.json` (`path`, `order`, `prerequisite`); the paths are defined in `src/lib/curriculum/paths.ts`. `/learn` groups by path and the home page shows the path name on each card.
- Four languages plus a kernel mode, all run in a bubblewrap sandbox (no network, CPU/memory/process limits): C (gcc, UBSan), Rust (rustc), Go (`go test`), x86-64 assembly (GNU `as`, Intel syntax, tested by hidden C) and `kernel` (the osdev track boots the learner's freestanding C under QEMU). Details in `CLAUDE.md`.
- `npm run verify:content` checks every lesson (solution passes, starter fails). Needs gcc, rustc, go, bubblewrap and qemu-system-x86_64.
- AI tutor (`src/lib/tutor.ts`, `/api/tutor`): Claude Haiku 4.5 by default, or Ollama (`TUTOR_PROVIDER=ollama`, CPU inference is slow without `ollama-cuda`), response cache, quotas per user/IP/global, pre-written failure explanations (`src/lib/diagnostics.ts`). Needs `ANTHROPIC_API_KEY` in `.env.local` or Ollama installed; check that it is set before relying on it.
- Auth: scrypt, hashed sessions in httpOnly SameSite=Strict cookies, TOTP 2FA + backup codes (required for admins), lockout with backoff, PoW CAPTCHA after 3 failures, HIBP + top-10k password checks, IP limits. `npm run test:e2e -- http://localhost:3100` passes (45 `check()` calls, against `next start -p 3100`).
- Gamification: XP (5/10/20/40 by difficulty), levels (level L needs 4·L·(L−1) XP, cap at level 10 = 360 XP, XP keeps counting), medieval titles, first-pass XP in `recordAttempt`, `/leaderboard` and `/u/[username]` public profiles (noindex, opt-out in settings). The header level chip refreshes after a first pass (`router.refresh()` in the workspace).
- Theme: dark default, light via OS or `data-theme`, tokens only (see `CLAUDE.md`). The sitewide look comes from the dashboard redesign (`docs/design/`): `.panel` surfaces, Archivo wide display headings (`.display`, `.display-sm`), amber only for progress and action, underlined active nav. Text contrast measured AA in both themes. Form-field borders use `--field` (3:1); `--border`/`--line` are decorative dividers only.
- Editor: failed compiles, panics and UBSan reports tint the offending lines (`src/lib/error-lines.ts`, parsed client-side from the run output). The active-line highlight is hidden while text is selected, otherwise it covers the last selected line.
- Review lessons and "last lesson" references link to the lessons they build on (`/learn/<track>/<slug>`). Add links the same way in new lessons.
- GDPR/SEO: legal pages from `content/legal/*.md` (privacy covers public profile data, cookies covers `bm:theme`), consent banner, first-party analytics, export/delete in settings, robots, sitemap, llms.txt, og:images, JSON-LD, canonical, skeletons, 404.

## Known gaps

- **`/u/<valid-name>` for a user that doesn't exist returns HTTP 200** with the not-found page. Malformed names return a real 404. A true 404 needs the existence check before streaming starts, which would make the page blocking; left as is because the pages are `noindex`. `generateStaticParams` in that page returns one sample name only to satisfy Cache Components at build time.
- **375px header** was checked only as static markup in a narrow frame (it wraps to two rows, nothing clipped, no horizontal scroll), not on a real phone-width viewport.
- **Gamification plan not built beyond XP/levels/profiles.** `docs/gamification-plan.md` has phases A–D. Phase D was rewritten to be solo-first (review queue, personal bests, daily pick, own-action achievements), with community features dormant until enough users exist; it proposes a `communityReady()` helper that does not exist yet. The user-count threshold is an open decision there.
- A rustc/LLVM linker abort ("terminated with signal 6") hit one Rust lesson once during `verify:content` and did not recur. If it comes back, note which lesson.
- Turbopack warns about dynamic file access at `src/lib/runner.ts:211`. Harmless so far.
- Not production-ready: needs a reverse proxy setting `X-Forwarded-For` (`TRUSTED_PROXY_HOPS`), SMTP, Mongo auth and legal review. See README "Before going to production".

## Suggested next steps

0. **Test-result integrity (`docs/security-audit.md` L1) is mitigated, not fixed.** Results carry a per-run nonce (C/asm/kernel), submissions containing harness protocol text are rejected, and first-pass XP is paced. A determined learner can still forge by reading the nonce out of their own process. The root fix is an out-of-process verdict (driver prints raw values, server diffs against a reference transcript), which means rewriting the lesson tests. The sandbox is also hardened (seccomp, no user namespaces, read-only work dir at run time, capped tmpfs, bounded queue); cgroup limits and uid separation are deployment tasks in `docs/deployment-hardening.md`.
1. Start Phase A of `docs/gamification-plan.md` (XP ledger, server-side hint tracking, learner timezone). Phase D's solo features (D1–D4) do not need a userbase.
2. Decide the community threshold (open decision in the plan) before building D5.
3. Real-device phone check of the header and lesson workspace (checked in headless Chromium at 390px only).
4. Docs worth reading: `docs/security-audit.md` (open audit plan, PoCs in `scripts/security/`), `docs/design/` (dashboard handoff and reference).

## Gotchas

- Node via nvm; Rust in `~/.cargo/bin`; MongoDB binaries in `~/.local/bin` (`npm run db`).
- After adding routes run `npx next typegen`, or `PageProps<"/route">` won't typecheck.
- Next 16 here differs from training data: read `node_modules/next/dist/docs/` before changing framework-level code (`AGENTS.md`).
- Pages that read the DB during render need `await connection()` inside `<Suspense>`.
- Lesson ids (`track/slug`) are the progress keys. Renumbering folder prefixes is safe; changing a slug or `id:` loses learners' progress. After changing a lesson's difficulty run `npm run recompute-xp`.
- Adding a language touches the runner, curriculum types, editor grammar, diagnostics and the tutor prompt.
- Rust here is installed system-wide (`/usr/bin/rustc`), not via rustup, so the runner finds no toolchain unless `RUST_TOOLCHAIN_DIR=/usr` is set (in `.env.local` for dev, and when running `verify:content`).
- Don't `pkill -f` with a pattern that also appears in your own command line (it kills the shell). Kill by PID.
- `verify:content -- osdev` reports a bare "triple fault" for every lesson when `qemu-system-x86_64` is missing; install QEMU before trusting a failure.
- Commits in this repo carry no co-author or "Generated with" lines.
