# Handoff — Baremetal

Status as of 27 October 2026. Read this, then `README.md` and `GETTING_STARTED.md`.

## What exists and works (verified)

- Next.js 16.4 app (App Router, Cache Components) + local MongoDB + Tailwind v4. Project: `~/code/baremetal`.
- 11 courses / 79 lessons in `content/` (C, Rust, Go and x86-64 assembly). `npm run verify:content` checks every lesson (solution passes, starter fails) — was all green.
- Sandboxed runner (`src/lib/runner.ts`): gcc/rustc inside bubblewrap, no network, CPU/mem/proc limits, UBSan for C.
- AI tutor (`src/lib/tutor.ts`, `/api/tutor`): Claude Opus 5.5 or Ollama (`TUTOR_PROVIDER`). Response cache, prompt caching, quotas per user/IP/global, pre-written failure explanations with "Elaborate" button (`src/lib/diagnostics.ts`). **Not configured yet:** needs `ANTHROPIC_API_KEY` in `.env.local` or Ollama installed.
- Auth hardening: scrypt, hashed sessions in httpOnly SameSite=Strict cookies, TOTP 2FA + backup codes (required for admins), lockout with exponential backoff, PoW CAPTCHA after 3 failures, HIBP + top-10k password checks, IP limits (trusted-proxy aware). `npm run test:e2e -- http://localhost:3100` → 45/45 passed (run against `next start -p 3100`).
- GDPR/SEO set: legal pages from `content/legal/*.md`, consent banner + first-party analytics, export/delete in settings, robots, sitemap, llms.txt, og:images, JSON-LD, canonical, skeletons, 404.

## Added this session (typecheck, lint and `next build` pass; NOT yet browser-tested or e2e re-run)

1. **Light mode** — tokens in `src/app/globals.css` (dark default, light via OS or `data-theme`), toggle `src/components/theme-toggle.tsx` in header, pre-paint script via `src/components/inline-script.tsx` (the pattern from Next's "preventing flash before hydration" guide — fixes the React "script tag" console error). Editor colours are `--code-*` tokens.
2. **Difficulty** — every `lesson.md` has `difficulty: easy|medium|hard|impossible` (8/13/6/2). Colours green/amber/red/dark-red (`--easy` … `--impossible`). Badge: `DifficultyBadge` in `src/components/gamification.tsx`, shown on lesson pages and track lists.
3. **XP & levels** — `src/lib/gamification.ts`: 5/10/20/40 XP; level L needs 4·L·(L−1) XP; 935 XP total; level 10 is reached at 360 XP and is the cap. Medieval titles: Novice … Archmage. XP granted once on first pass (`recordAttempt` in `src/lib/progress.ts`, race-safe), returned by `/api/run` as `reward` → "+XP / Level up" banner. `npm run recompute-xp` rebuilds XP from progress (already run once; your account got backdated XP).
4. **Usernames & profiles** — `username` (unique, lowercase, `usernameProblem()` rules) on sign-up and in settings; `profilePublic` toggle (default on). Pages: `/leaderboard` (top 50 + username search) and `/u/[username]` (`src/lib/community.ts`). Both `noindex`. Header shows level chip; dashboard shows level card + "choose a username" prompt for old accounts.

## Next steps (in order)

1. ~~Build~~ done (105 pages). `<Analytics />` is now inside `<Suspense>` in the root layout — needed for `/u/[username]`.
2. Browser-check (dev server): toggle theme on home + a lesson (editor readable in light), no console errors; pass a lesson → "+XP" banner; set a username in /settings → appears on /leaderboard and /u/<name>; hide profile → 404 + gone from board.
3. `scripts/e2e-auth.mts` already sends a `username` on sign-up — just re-run it (`npx next start -p 3100`, then `npm run test:e2e -- http://localhost:3100`).
4. Privacy notice (`content/legal/privacy.md`): add a row for public profile data (username, level, XP, completed lessons; basis: contract; visible unless hidden in settings). Cookie policy (`content/legal/cookies.md`): mention the `bm:theme` localStorage preference.
5. ~~CLAUDE.md~~ done — design rules live there (no theme file existed before; the current look is the theme). `GETTING_STARTED.md` has the setup steps.
6. `npm run verify:content` once more (loader now requires `difficulty`).
7. Commit (nothing committed yet): `git add -A && git commit -m "Baremetal: courses, tutor, auth, gamification"`.

## Gotchas

- Node via nvm; Rust in `~/.cargo/bin`; MongoDB binaries in `~/.local/bin` (`npm run db`). Shell PATH was fixed in `~/.zshrc`/`~/.bashrc`.
- After adding routes, run `npx next typegen` or `PageProps<"/route">` won't typecheck.
- Next 16 here differs from training data — read `node_modules/next/dist/docs/` before changing framework-level code (see `AGENTS.md`).
- Don't `pkill -f` with a pattern that's also in your own command line (it kills the shell). Kill by PID.
- Production needs a reverse proxy setting `X-Forwarded-For` (`TRUSTED_PROXY_HOPS`), SMTP, Mongo auth, legal review — see README "Before going to production".
