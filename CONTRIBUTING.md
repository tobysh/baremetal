# Contributing to Baremetal

Thanks for helping. This page covers setting up, the checks to run, and the rules that keep the site safe and the curriculum honest. To understand the system first, read [docs/architecture.md](docs/architecture.md).

## What you can work on

| Kind of change | Start with |
|---|---|
| New or improved lessons | [docs/content-authoring.md](docs/content-authoring.md) |
| Web app, editor, dashboard | [docs/architecture.md](docs/architecture.md), then "UI rules" below |
| Runner, sandbox, tutor, auth | [docs/security.md](docs/security.md) first; these areas carry the most risk |
| Gamification (XP, levels, achievements) | [docs/gamification.md](docs/gamification.md) |
| Deployment, ops | [docs/deployment-hardening.md](docs/deployment-hardening.md) |

## Set up

Baremetal is Linux-first. You need Node.js 22+, MongoDB, and for running exercises: gcc, Rust, Go, [bubblewrap](https://github.com/containers/bubblewrap) and `qemu-system-x86_64` (kernel lessons). Without the toolchains the site still runs, but the matching exercises can't.

```bash
git clone https://github.com/tobysh/baremetal.git && cd baremetal
npm ci
mkdir -p .data/db
```

Create `.env.local` (see [docs/configuration.md](docs/configuration.md) for everything configurable):

```dotenv
MONGODB_URI=mongodb://127.0.0.1:27017
MONGODB_DB=baremetal
AUTH_SECRET=<output of: openssl rand -base64 48>
```

Run MongoDB and the app in two terminals:

```bash
npm run db      # mongod, data in .data/db
npm run dev     # http://localhost:3000
```

Make yourself an admin with `npm run make-admin -- you@example.com` (needed for `/admin`). In development no runner service is needed: `/api/run` executes exercises in-process (still inside bubblewrap).

## Before you open a pull request

Always:

```bash
npx next typegen        # after adding or renaming routes
npx tsc --noEmit
npm run lint
npm run check:theme
```

Then whatever your change touches:

| If you changed… | Run |
|---|---|
| Lessons, tracks, or anything under `content/` | `npm run verify:content -- <track>`, then `npm run check:ids`, `npm run check:learning`, `npm run check:curriculum` |
| A lesson's difficulty or `LEVEL_FLOORS` | `npm run recompute-xp` against any database with real learners (site stopped) |
| Runner, sandbox, harness scanning | `npm run test:runner`, `node --import tsx scripts/security/integrity/fixtures-forgery.mts`, `node --import tsx scripts/security/runner-cancellation.mts`, `npm run test:sandbox` |
| Pure helpers (`redirects`, `totp`, `crypto`, `rate-limit`, `tutor-normalize`) | `npm run test:unit` |
| Anything stored (collections, cookies, localStorage keys) or `content/legal/` | `npm run check:legal`; update `content/legal/` and bump `SITE.legal.lastUpdated` |
| Auth, API routes, rate limits | `npm run build && npx next start -p 3100` then `npm run test:e2e -- http://localhost:3100` |
| Tutor | `scripts/tutor/*.mts` (stream guard, cost, reservations) |
| Workspace/UI behaviour | `npm run test:browser` (Playwright; `npx playwright install chromium`) |
| Anything user-visible end to end | `npm run test:regressions` (builds an isolated production copy with its own scratch DB; needs MongoDB) |
| Dependencies | `npm audit --omit=dev` |

CI (`.github/workflows/ci.yml`) runs typecheck, lint, id/theme/learning/curriculum guards, a build, the full content verification (with real toolchains), the forgery and cancellation fixtures, the runner transport suite, and the isolated regression and browser suite. The full set is what must pass to merge. There is no unit-test framework: checks are scripts under `scripts/` that exit non-zero on failure. To run one check on its own, edit the script or run it directly with `npx tsx`.

Scratch scripts that import `mongodb` or repo code only resolve from inside `scripts/`; delete them afterwards.

## Rules that are easy to break

**Security.** Learner code runs only inside the sandbox. Never add a path that runs it elsewhere or widens the sandbox. Hidden tests and reference solutions must never reach the browser or the tutor: the client gets `toPublicExercise` output only. Keep security lessons synthetic. Changes to the runner, auth, tutor guard, XP or rate limits deserve a regression check in `scripts/` (add a `check()` for the case you fixed).

**Lesson ids are progress keys.** Do not rename a published lesson's slug or `id:`. `npm run check:ids` enforces this.

**XP integrity.** XP is granted once, in `recordAttempt`, and logged in `xp_events`. Do not add other XP paths. `recompute-xp` rewrites ledgers per user and must **never run while the site is serving passes**.

**Privacy and legal.** If you add stored data, a cookie, or a `localStorage` key, update `content/legal/*.md`, the account export and deletion code, and bump `SITE.legal.lastUpdated` in `src/lib/site.ts`.

**Next.js 16.** Behaviour differs from older versions. Read the relevant file in `node_modules/next/dist/docs/` before framework-level changes. Pages that read cookies, the DB or dynamic params belong inside `<Suspense>`; those reading MongoDB during render call `await connection()` first.

### UI rules

- **Colours come from tokens only** (`src/app/globals.css`): `bg`, `surface`, `surface-2`, `border`, `text`, `muted`, `accent`, `ok`, `err`, `--code-*`. Never hard-code hex values in components; add a token with both a dark and a light value instead. `npm run check:theme` enforces this.
- Cards and callouts use a full border and a tint, **never a left accent stripe**.
- Text and UI stay at WCAG AA contrast in both themes. Respect `prefers-reduced-motion`. Every interactive element has a keyboard path and an accessible name.
- One `<h1>` per page; every page sets its own `title`, `description` and `alternates.canonical`.
- Difficulty uses `DifficultyBadge` (Easy green, Medium amber, Hard red, Impossible dark red). Level titles are medieval and never demeaning.
- Lazy-load heavy client code (editor, terminal, password checker) and check bundle impact after adding a dependency.

## Commits and pull requests

- Write plain, descriptive commit messages. Group related changes into one commit and unrelated ones into separate commits.
- **Do not add AI attribution** to commits or PR descriptions (no `Co-Authored-By` trailers, no "Generated with…" lines, no session links).
- Don't push to `main` directly or deploy from a pull request. Don't commit `.env*`, `.data/`, backups, or anything containing secrets.
- Describe what changed and how you verified it. If a check could not be run (no KVM, no QEMU, no real host), say so rather than implying it passed.

## Working with AI coding agents

`CLAUDE.md` and `AGENTS.md` at the repo root carry the project rules for coding agents; they point back to the docs in this repository rather than duplicating them. Treat agent output like any other contribution: run the checks above and review the diff.

## Operational tips

- Don't `pkill -f` with a pattern that also appears in your own command line; kill by PID.
- `npm run verify:content -- osdev` reports a bare "triple fault" per lesson if `qemu-system-x86_64` is missing.
- Rust is auto-detected from the system; `RUST_TOOLCHAIN_DIR` overrides it. Go can be pointed at a toolchain with `GO_TOOLCHAIN_DIR`.
- Some security checks have special runtime conditions: run `scripts/security/quick-fixes.mts` with Node's `react-server` condition (`node --conditions=react-server --import tsx …`) and the renderer fixtures (`scripts/security/markdown.mts`) without it.
