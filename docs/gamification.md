# Gamification

Current design of XP, levels, seals, challenges, reviews and the Codex. Implemented: everything below under "What exists". Not built: community seasons and rank ("Not built" at the end).

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
| Challenges | Optional harder variants, unlocked after the lesson pass, half XP. Stored as progress docs with id `<lesson id>#challenge`. Files: `challenge.md`, `challenge-starter.*`, `challenge-solution.*`, `challenge-tests.*` or `challenge-cases.json`. `verify:content` checks the solution passes and the plain lesson solution fails. Currently 22; list them with `ls content/*/*/challenge.md`. |
| Practice | Optional tap-and-drag exercises (parsons, blanks, predict, bug) from a lesson's `practice.json`, a quarter of the lesson's XP with a minimum of 1 (easy 1, medium 2, hard 5, impossible 10; `practiceXpFor`). Stored as progress docs with id `<lesson id>#practice`, ledger source `practice`. Granted once, on first pass, through `recordAttempt` with `practice: true`, so the pace limit, the rolling 24 h cap and the minimum solve time (a flat 4 s for practice) apply like any pass. No seals, no review schedule, no Codex credit. A practice pass counts toward the weekly rhythm. It never blocks the lesson's own full XP. |
| Codex | 20 achievements, no XP, defined in `src/lib/achievements.ts` (`id`, `name`, `description`, `check`). Adding one needs `npm run backfill-achievements`. |
| Weekly rhythm | `src/lib/rhythm.ts`, derived from `passedAt` (withheld passes excluded). Goal 1/2/3/5 or none, timezone; only `users.weeklyGoal` and `users.timezone` are stored. Rest weeks banked every 4 met weeks (max 2), no loss messaging. |
| Reviews | Spaced re-solves at 3/7/21/60 days, no XP. Every due date gets a random spread of ±10% (at least ±1 day from 7 days up; `jitterDays` in `src/lib/review-math.ts`), and the stored `interval` stays the nominal step. Failed runs never reset the schedule; only "I need a refresher" does. `/review` shows up to 20 due lessons as graded question cards and posts to `/api/review`. Each card asks one question from the lesson's deck (its `practice.json` plus `quiz.json`; the question changes each round) in any practice kind, checked on the server. The first answer settles the schedule: right advances a due review, wrong restarts at 3 days and reveals the answer. A card can be answered once per due date. After a miss the learner gets one more question from the same deck (a one-question deck brings the missed question back at the end of the session); retries never change the schedule and are logged with source `retry`. A lesson with no question gets a "write it again" card linking to its coding review, and "Skip until tomorrow" calls the server, which moves `dueAt` to the start of tomorrow in the learner's timezone (UTC if unset) and leaves the interval alone. The dashboard "Ready for review" list and daily pick link to `/review`; `?review=1` still opens the coding review. Cards do not increment the review `count`, so they never count toward Codex review achievements; only a solved review does. | Each card settle, coding pass and skip appends a row to `review_events` (rating again/hard/good/easy, days late, days since the previous review, interval before and after); rows are written only when the schedule actually moved.
| UI | Course map, stats, daily pick, level-up and course-complete dialogs, achievement toasts, profile showcase, `/leaderboard` (learners with XP and a public username only) and `/u/[username]` (noindex, opt-out). |

Collections: `xp_events`, `achievements` (unique `(userId, id)`), `reviews` (unique `(userId, exerciseId)`), `review_events` (index `(userId, at)`); progress fields `hintsUsed`, `hintsUsedAtPass`, `attemptsAtPass`; user fields `timezone`, `weeklyGoal`. All are in data export and account deletion and listed in `content/legal/privacy.md`.

## Maintenance rules

- Changing `LEVEL_FLOORS`, a lesson's difficulty, challenge XP or practice XP, or adding or removing a `practice.json` (a removed one stops paying on recompute), needs `npm run recompute-xp` (rebuilds the ledger and review schedule per user). Run it only with the site stopped.
- On an existing database, run `recompute-xp` then `backfill-achievements` once.

## Not built (D5, deliberately)

`communityReady()` gate (public learners with a pass, threshold proposed at 20, env override), seasons (This month / All time from the ledger), rank on profiles only above the threshold. Decide the threshold from a real cohort before building. Out of scope: friends, duels, comments, XP gifting, paid cosmetics (need moderation). Open owner decisions: threshold, Season Laurels, whether achievement names show on public profiles by default.
