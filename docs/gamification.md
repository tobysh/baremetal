# Gamification

Current design. The original phased plan, with full rationale, is `docs/archive/gamification-plan.md`. Phases A to D4 are built; D5 (community features) is not.

## Principles

1. Reward learning, not grinding: XP only from passing tests and challenges.
2. Never punish asking for help: hints and the tutor are free and never reduce XP.
3. Celebrate perseverance as well as speed.
4. No anxiety mechanics: no loss-framed streaks, countdowns or guilt notifications.
5. The server is the source of truth; client signals only unlock cosmetics.
6. XP stays rebuildable from stored data (`npm run recompute-xp`).
7. New public data appears only with `profilePublic`, and is covered by privacy notice, export and deletion.
8. House style: tokens only, WCAG AA, medieval titles never demeaning, `prefers-reduced-motion` respected.

## What exists

| Feature | Where / how |
|---|---|
| XP | 5/10/20/40 by difficulty, granted once on first pass in `recordAttempt` (`src/lib/progress.ts`); ledger `xp_events`, unique `(userId, source, ref)`. Pace and 24 h caps: see `docs/security.md`. |
| Levels | `LEVEL_FLOORS` in `src/lib/gamification.ts`: 8, 24, 60, 120, 210, 330, 500, 720, 1000. Level 10 (Archmage) is the cap; XP keeps counting. Novice to Archmage titles with flavour lines. |
| Hints | Fetched and recorded via `/api/hint` (`progress.hintsUsed`); `hintsUsedAtPass` and `attemptsAtPass` snapshotted on first pass. |
| Seals | Passed (lesson XP), Unaided (no hints, no tutor before pass, and a failed run or hard+ difficulty; cosmetic), Challenge. |
| Challenges | Optional harder variants, unlocked after the lesson pass, half XP. Stored as progress docs with id `<lesson id>#challenge`. Files: `challenge.md`, `challenge-starter.*`, `challenge-solution.*`, `challenge-tests.*` or `challenge-cases.json`. `verify:content` checks the solution passes and the plain lesson solution fails. Currently 21; list them with `ls content/*/*/challenge.md`. |
| Codex | 20 achievements, no XP, defined in `src/lib/achievements.ts` (`id`, `name`, `description`, `check`). Adding one needs `npm run backfill-achievements`. |
| Weekly rhythm | `src/lib/rhythm.ts`, derived from `passedAt` (withheld passes excluded). Goal 1/2/3/5 or none, timezone; only `users.weeklyGoal` and `users.timezone` are stored. Rest weeks banked every 4 met weeks (max 2), no loss messaging. |
| Reviews | Spaced re-solves at 3/7/21/60 days, no XP. Failed runs never reset the schedule; only "I need a refresher" does. |
| UI | Course map, stats, daily pick, level-up and course-complete dialogs, achievement toasts, profile showcase, `/leaderboard` and `/u/[username]` (noindex, opt-out). |

Collections: `xp_events`, `achievements` (unique `(userId, id)`), `reviews` (unique `(userId, exerciseId)`); progress fields `hintsUsed`, `hintsUsedAtPass`, `attemptsAtPass`; user fields `timezone`, `weeklyGoal`. All are in data export and account deletion and listed in `content/legal/privacy.md`.

Deviations from the original plan: challenges as `#challenge` progress docs, derived rhythm, no `bestMs` (no timing data), `fifty-runs-unaided` became `unaided-25`.

## Maintenance rules

- Changing `LEVEL_FLOORS`, a lesson's difficulty or challenge XP needs `npm run recompute-xp` (rebuilds the ledger and review schedule per user). Run it only with the site stopped.
- On an existing database, run `recompute-xp` then `backfill-achievements` once.

## Not built (D5, deliberately)

`communityReady()` gate (public learners with a pass, threshold proposed at 20, env override), seasons (This month / All time from the ledger), rank on profiles only above the threshold. Decide the threshold from a real cohort before building. Out of scope: friends, duels, comments, XP gifting, paid cosmetics (need moderation). Open owner decisions: threshold, Season Laurels, whether achievement names show on public profiles by default.
