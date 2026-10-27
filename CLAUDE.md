# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

# Baremetal — project rules

Baremetal teaches C, Rust, Go and x86-64 assembly: markdown lessons, an in-browser editor, hidden tests run in a sandbox, and an AI tutor. Stack: Next.js 16.4 (App Router, Cache Components, Turbopack), Tailwind v4, local MongoDB (`MONGODB_DB=baremetal`). `README.md` has setup and production notes.

## Commands

- `npm run db` (MongoDB, data in `.data/db`) and `npm run dev` (port 3000) in two terminals. Settings are in `.env.local`.
- `npm run build`, `npm run lint`, `npx tsc --noEmit`. After adding routes run `npx next typegen`, or `PageProps<"/route">` won't typecheck.
- `npm run verify:content [-- <track>]` checks that every lesson's solution passes and its starter compiles but fails. It needs gcc, rustc, go, bubblewrap and `qemu-system-x86_64` (the `osdev` track boots a kernel per test, about 0.2 s each).
- `npm run test:e2e -- http://localhost:3100` runs the auth and security checks (45 `check()` calls in `scripts/e2e-auth.mts`) against `npx next start -p 3100`. There is no unit-test framework. To run one check, edit the script.
- `npm run recompute-xp` rebuilds XP from progress; `npm run make-admin -- <email>`.
- Don't `pkill -f` with a pattern that also appears in your own command line. Kill by PID.

## Architecture

- **Content pipeline:** `src/lib/curriculum/` loads `content/<track>/track.json` plus each lesson folder (`lesson.md` frontmatter, `task.md`, `hints.md`, `starter.*`, `solution.*`, and `tests.*` or `cases.json`). The track's `language` (`c`, `rust`, `go`, `asm` or `kernel`) picks the file extensions (`LANGUAGE_FILE`) and the labels (`LANGUAGE_LABEL`). Lesson ids are `track/slug` and are the key in the `progress` collection, so renaming a lesson loses learners' progress.
- **Runner:** `src/lib/runner.ts` (called from `api/run`) compiles and runs code in bubblewrap with no network, a read-only system and 5 s / memory / process limits. Tests are hidden harnesses using a shared `CHECK` macro. For assembly, `starter.s` is linked with a hidden C `tests.c` that has its own `main`. For Go, `main.go` is built in a throwaway module (CGO off, GOPROXY off, build cache in the sandbox `/tmp`, own 60 s compile budget because the std library is compiled cold, about 3 s) and the hidden `tests.go` becomes `bm_test.go`, a `go test -c` binary run with `-test.v`.
- **Kernel language:** the `osdev` track uses `"language": "kernel"`. The learner's `kernel.c` (freestanding x86-64 C, no libc) is linked with the hidden support files in `src/lib/kernel/` (`boot.S` Multiboot stub into long mode with the first GiB identity-mapped and page 0 unmapped; `runtime.c` with port I/O, weak `mem*`, `bm_printf` and a default exception reporter; `link.ld`) and booted with `qemu-system-x86_64 -kernel` (TCG, no disk or network) inside bubblewrap. `tests.c` is appended to the learner's file and defines `kmain()` (the learner's own `kmain` is renamed with `-Dkmain`). Test results go over the QEMU debug console (port 0xE9) in the same `@@RUN/@@PASS/@@FAIL` protocol, so they never mix with COM1, which is the learner's stdout. Exit is `isa-debug-exit`; exit status 1 is success, a CPU exception prints `@@EXC` and exits, and a bare reset is reported as a triple fault. Tests inspect real hardware state (UART loopback, VGA memory, CRTC, PIC, PIT interrupts, live page tables, `invlpg` behaviour), so keep timing assertions loose.
- **Failure explanations and tutor:** `src/lib/diagnostics.ts` holds pre-written regex-matched explanations per language. `src/lib/tutor.ts` is the Claude/Ollama tutor with caches and quotas. A new language touches the runner, curriculum types, editor grammar, diagnostics and the tutor prompt.
- **XP:** `src/lib/gamification.ts` (5/10/20/40 per difficulty, level L needs 4·L·(L−1) XP, capped at level 10, XP keeps counting). XP is granted once, on first pass, in `recordAttempt` (`src/lib/progress.ts`). Changing a lesson's difficulty needs `recompute-xp`.
- **Auth:** `src/lib/auth.ts` with scrypt, hashed session tokens in the `bm_session` cookie, TOTP, lockout and PoW CAPTCHA. `src/proxy.ts` is the Next 16 middleware.
- **Public profiles:** `src/lib/community.ts` serves `/leaderboard` and `/u/[username]`. Pages that read the DB during render must `await connection()` from `next/server` inside `<Suspense>`, otherwise the driver's `new Date()` fails prerendering.
- **Legal pages** come from `content/legal/*.md`. Update them when a change adds stored data or a cookie/localStorage key.

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

- Lessons live in `content/<track>/<NN-slug>/`, and each `lesson.md` needs `difficulty:`. Run `npm run verify:content` after editing lessons, and `npm run recompute-xp` after changing a difficulty.
