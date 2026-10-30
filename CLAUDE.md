# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

# Baremetal — project rules

Baremetal teaches C, Rust, Go and x86-64 assembly: markdown lessons, an in-browser editor, hidden tests run in a sandbox, and an AI tutor. Stack: Next.js 16.4 (App Router, Cache Components, Turbopack), Tailwind v4, local MongoDB (`MONGODB_DB=baremetal`). `README.md` has setup and production notes.

## Commands

- `npm run db` (MongoDB, data in `.data/db`) and `npm run dev` (port 3000) in two terminals. Settings are in `.env.local`.
- `npm run build`, `npm run lint`, `npx tsc --noEmit`. After adding routes run `npx next typegen`, or `PageProps<"/route">` won't typecheck.
- `npm run verify:content [-- <track>]` checks that every lesson's solution passes and its starter compiles but fails. It needs gcc, rustc, go, bubblewrap and `qemu-system-x86_64` (the `osdev` track boots a kernel per test, about 0.2 s each).
- `npm run test:e2e -- http://localhost:3100` runs the auth and security checks (the expanded checks in `scripts/e2e-auth.mts`) against `npx next start -p 3100`. There is no unit-test framework. To run one check, edit the script.
- `npm run recompute-xp` rebuilds XP from progress; `npm run make-admin -- <email>`.
- Don't `pkill -f` with a pattern that also appears in your own command line. Kill by PID.

## Architecture

- **Content pipeline:** `src/lib/curriculum/` loads `content/<track>/track.json` plus each lesson folder (`lesson.md` frontmatter, `task.md`, `hints.md`, `starter.*`, `solution.*`, and `tests.*` or `cases.json`). The track's `language` (`c`, `rust`, `go`, `asm` or `kernel`) picks the file extensions (`LANGUAGE_FILE`) and the labels (`LANGUAGE_LABEL`). Lesson ids are `track/slug` and are the key in the `progress` collection, so renaming a lesson loses learners' progress.
- **Runner:** `src/lib/runner.ts` (called from `api/run`) compiles and runs code in bubblewrap with no network, a read-only system and 5 s / memory / process limits. Tests are hidden harnesses using a shared `CHECK` macro. For assembly, `starter.s` is linked with a hidden C `tests.c` that has its own `main`. For Go, `main.go` is built in a throwaway module (CGO off, GOPROXY off, build cache in the sandbox `/tmp`, own 60 s compile budget because the std library is compiled cold, about 3 s) and the hidden `tests.go` becomes `bm_test.go`, a `go test -c` binary run with `-test.v`.
- **Kernel language:** the `osdev` track uses `"language": "kernel"`. The learner's `kernel.c` (freestanding x86-64 C, no libc) is linked with the hidden support files in `src/lib/kernel/` (`boot.S` Multiboot stub into long mode with the first GiB identity-mapped and page 0 unmapped; `runtime.c` with port I/O, weak `mem*`, `bm_printf` and a default exception reporter; `link.ld`) and booted with `qemu-system-x86_64 -kernel` (TCG, no disk or network) inside bubblewrap. `tests.c` is appended to the learner's file and defines `kmain()` (the learner's own `kmain` is renamed with `-Dkmain`). Test results go over the QEMU debug console (port 0xE9) in the same `@@RUN/@@PASS/@@FAIL` protocol, so they never mix with COM1, which is the learner's stdout. Exit is `isa-debug-exit`; exit status 1 is success, a CPU exception prints `@@EXC` and exits, and a bare reset is reported as a triple fault. Tests inspect real hardware state (UART loopback, VGA memory, CRTC, PIC, PIT interrupts, live page tables, `invlpg` behaviour), so keep timing assertions loose.
- **Failure explanations and tutor:** `src/lib/diagnostics.ts` holds pre-written regex-matched explanations per language. `src/lib/tutor.ts` is the Claude/Ollama tutor with caches and quotas. A new language touches the runner, curriculum types, editor grammar, diagnostics and the tutor prompt.
- **XP:** `src/lib/gamification.ts` (5/10/20/40 per difficulty, cumulative XP per level is the `LEVEL_FLOORS` table, 8 and 24 then steeper to 1000 at level 10, XP keeps counting). XP is granted once, on first pass, in `recordAttempt` (`src/lib/progress.ts`), and logged in the `xp_events` ledger. Seals, challenges (`<id>#challenge` exercises), the Codex (`src/lib/achievements.ts`), weekly rhythm (`src/lib/rhythm.ts`) and reviews are described in `docs/gamification.md` (original plan in `docs/archive/`). Changing a lesson's difficulty needs `recompute-xp`.
- **Auth:** `src/lib/auth.ts` with scrypt, hashed session tokens in the `bm_session` cookie, TOTP, lockout and PoW CAPTCHA. `src/proxy.ts` is the Next 16 middleware.
- **Public profiles:** `src/lib/community.ts` serves `/leaderboard` and `/u/[username]`. Pages that read the DB during render must `await connection()` from `next/server` inside `<Suspense>`, otherwise the driver's `new Date()` fails prerendering.
- **Legal pages** come from `content/legal/*.md`. Update them when a change adds stored data or a cookie/localStorage key, and bump `SITE.legal.lastUpdated` in `src/lib/site.ts` whenever anything in `content/legal/` changes.

Also read `AGENTS.md` (Next.js 16 differs from training data; read `node_modules/next/dist/docs/` first) and `HANDOFF.md`.

## Theme

- **Colours come from tokens only** (`src/app/globals.css`): `bg`, `surface`, `surface-2`, `border`, `text`, `muted`, `accent`, `ok`, `err`, plus `--code-*` for the editor. Never hard-code hex values in components. Every token has a dark value (default) and a light value (OS preference or `data-theme="light"`); add both when adding a token.
- **Dark:** ink background `#0e1013`, amber accent `#f5a524`. **Light:** warm paper `#fbfaf7`, amber `#b86e00`.
- **Fonts:** IBM Plex Sans for UI, IBM Plex Mono for code.
- **Difficulty colours:** Easy green (`--easy`), Medium amber (`--medium`), Hard red (`--hard`), Impossible dark red (`--impossible`, a solid badge). Use `DifficultyBadge`.
- **Level titles** are medieval and never demeaning: Novice → Archmage.
- Cards and callouts use a full border and a tint — **never a left accent stripe**.
- Text and UI must stay at WCAG AA contrast in both themes.

## Pages

- One `<h1>` per page. Every page sets its own `title`, `description` and `alternates.canonical`.
- Request-time data (cookies, DB, params on dynamic routes) goes inside `<Suspense>` with a skeleton fallback.
- Keep heavy client code (editor, password checker) lazy-loaded; check bundle impact after adding dependencies.

## Content

- Lessons live in `content/<track>/<NN-slug>/`, and each `lesson.md` needs `difficulty:`. Optional frontmatter: `tags:` (comma-separated, lowercase kebab-case, shown as `#tag` chips) and `kind: review`. A review lesson is a consolidation exercise: no new concepts, familiar ideas combined with small twists, kept comfortable (usually `easy`/`medium`), titled `Review: …`. Insert one by renumbering the folder prefixes; the slug and `id:` are unaffected, so progress is kept. Run `npm run verify:content` after editing lessons, and `npm run recompute-xp` after changing a difficulty.
- **No hardcodable answers.** A `mode: stdout` lesson whose starter declares its inputs (`int minutes = 275;`) must not pass by printing the expected text. Add extra cases to `cases.json` with `"set": {"minutes": "500"}`: the runner rewrites the first declaration of each named variable (`=`, `:=`, type annotations and comma declarations are handled; quoted values are kept intact) and recompiles before running, and the case fails with a "keep the starter's variables" message if a variable is missing. Use at least two variants, include edge values (0, a remainder of 0, negatives) and tell the learner in `task.md` that other values are tried. Lessons that read `stdin` vary it per case instead, and harness lessons (`tests.*`) must call the learner's code with several different inputs. Stdout lessons with no input to vary (fixed text, Go/Rust hello lessons where the learner names the variables, FizzBuzz, the star triangle) can't be protected this way. Check this whenever a new stdout lesson is added.

## Recovery security changes (29 October 2026)

Security model and open limitations are in `docs/security.md`; deployment items in `docs/deployment-hardening.md`. Hints are fetched and recorded through `/api/hint`; public exercises include only `hintCount`. The tutor never receives the reference solution and streams output through `SolutionGuard`, which withholds the last 24 tokens until each window is checked against the solution. Run `scripts/security/quick-fixes.mts` with Node's `react-server` condition; renderer checks run without that condition. Compiler work now uses capped tmpfs and returns a capped artifact pipe; run directories are read-only. `RUNNER_CGROUP_ROOT` enables delegated per-job cgroup-v2 limits, which need host verification. `safeNext` lives in `src/lib/redirects.ts` and is re-exported from auth. New signup uses email verification; `scripts/deliver-email.mts` retries the encrypted queue. Never run `recompute-xp` while the site is serving passes.
