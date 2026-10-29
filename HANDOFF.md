# Handoff

Dev environment, as of 29 October 2026. Read `CLAUDE.md` (architecture, commands, theme and content rules) and `README.md` first. Current state of the big subsystems: `docs/security.md`, `docs/gamification.md`. Old handoffs and finished plans: `docs/archive/`.

## State

- Next.js 16.4 app, four languages plus kernel mode, hidden-test sandbox, AI tutor, auth with 2FA, full gamification (phases A to D4). Course and lesson counts change often; `npm run verify:content` is the authority and passed on the full curriculum during the 29 October recovery.
- Last full check set passed: production build, auth e2e, API flood/quota/hint suite, XP race and forged-result fixtures, renderer fixtures, TypeScript, and production `npm audit` (zero advisories). Lint has zero errors and one unused-variable warning in `scripts/security/integrity/poc-fakepass.mts`.
- The recovery of the interrupted content and security work was done in the working tree without commits; the original security worktrees and branches still exist.

## Before production

One section on purpose: see `docs/deployment-hardening.md` (proxy and `TRUSTED_PROXY_HOPS`, HTTPS, secrets, SMTP plus the email worker, Mongo auth, cgroups on the real host, runner uid, legal details). Known limits that stay after that (same-process test forgery, `unsafe-inline` CSP) are in `docs/security.md`.

## Known gaps

- `/u/<valid-name>` for a nonexistent user returns HTTP 200 with the not-found page (a real 404 would make the page blocking; the pages are noindex). `generateStaticParams` returns one sample name only to satisfy Cache Components.
- The 375px header was only checked as static markup, not on a real phone-width viewport.
- Gamification phase D5 (community, `communityReady()`) is not built, on purpose; the threshold is an open owner decision.
- Intermittent rustc/LLVM linker address-space aborts appeared during recovery; limiting lld to one worker fixed the repeats without relaxing limits.
- Turbopack warns about dynamic file access at `src/lib/runner.ts` (harmless so far).

## Next steps

1. On any existing database: `npm run recompute-xp` (site stopped), then `npm run backfill-achievements`. Level thresholds changed, so levels drop; XP does not.
2. Click through the new UI in a real browser at 1440px and 390px (lesson, challenge and review tabs, both celebration dialogs for focus trap, Escape and reduced motion, toasts), then a real phone. Playwright with headless Chromium works; set `CHROME=` to the `chromium_headless_shell` under the Playwright browsers dir.
3. Tutor budget and cache work is implemented: atomic UTC-day reservations in `tutor_spend`, configurable estimated pricing, privacy-checked shared responses, and longer retention for popular answers. See README for variables and validation scripts.
4. Content: more challenges (only 4), arrays/strings/structs before C memory, more Go (context, `net/http`, testing), intermediate Rust (traits, lifetimes, errors, concurrency).
5. Admin view of where learners get stuck (pass rates, hints, tutor use) and a screen-reader pass over the dialogs and toasts.
6. Review the unaudited files listed in `docs/security.md`.

## Gotchas

- Node via nvm; Rust is system-wide (`/usr/bin/rustc`, auto-detected, `RUST_TOOLCHAIN_DIR` overrides); MongoDB binaries in `~/.local/bin` (`npm run db`).
- After adding routes run `npx next typegen`. Pages that read the DB during render need `await connection()` inside `<Suspense>`.
- Next 16 differs from training data: read `node_modules/next/dist/docs/` before framework-level changes (`AGENTS.md`).
- Lesson ids (`track/slug`) are progress keys; renumbering folder prefixes is safe, changing a slug or `id:` loses progress. Changing `LEVEL_FLOORS`, a difficulty or challenge XP needs `recompute-xp`, which deletes and reinserts the ledger per user and must never run while the site serves passes.
- `verify:content -- osdev` reports a bare "triple fault" for every lesson when `qemu-system-x86_64` is missing.
- Scratch scripts that import `mongodb` or repo code only resolve from inside `scripts/`; delete them afterwards.
- Don't `pkill -f` with a pattern in your own command line; kill by PID.
- Run `scripts/security/quick-fixes.mts` with Node's `react-server` condition; renderer checks without it.

## Interrupted-work recovery

The curriculum has 35 tracks, 423 lessons (38 reviews), and 4,970 base lesson XP, before challenges. Completed the missing ethical-hacking capstone harness; its absence caused curriculum loading to throw for every exercise, breaking Run and autosave. The 20 ethical-hacking lessons verify, including 148 capstone assertions. Existing lesson ids were retained.

CI, id/theme guards and backup/restore files were recovered from Claude's completed local branches into the working tree. The id snapshot now contains all 423 lessons. Tutor budgets were reconciled into the existing `tutor_spend` implementation rather than adding the branch's competing spend collection. Nested Claude worktrees are excluded from project lint/typecheck. The build can be verified with webpack when Turbopack worker port binding is restricted.

No existing difficulty changed during this recovery, so no XP recomputation was required. No production database recomputation, deployment or push was performed. Host-dependent runner isolation, cgroups, SMTP delivery and off-host backup remain deployment tasks. Anonymous hints cannot prove a learner never received help; the Unaided seal still relies partly on honesty.

Recovery validation passed: all 423 lesson starters/solutions (including challenges), webpack production build, TypeScript, lint (one pre-existing warning), id/theme guards, auth/security e2e, live leap-year Run/Test/autosave, sticky tutor use, concurrent budget reservations, and scratch backup/restore with BSON/index preservation. No new commits or attribution trailers were added.
