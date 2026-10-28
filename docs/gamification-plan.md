# Gamification plan: phase 2

> **Status (28 Oct 2026): phases A to D4 are built.** D5 (community features gated by user count) is deliberately not built.
> Deviations from the text below:
> - **Level curve:** `LEVEL_FLOORS` in `gamification.ts` (8, 24, 60, 120, 210, 330, 500, 720, 1000), steeper than the old 4·L·(L−1). Level 10 stays the cap; XP keeps counting.
> - **Challenges** are stored as their own `progress` documents with the exercise id `<lesson id>#challenge` (so they get saved code and attempts for free), not as a `progress.challenge` field. There is one per language track family: `c-memory/linked-lists`, `rust-foundations/vectors-and-iterators`, `go-foundations/slices-and-maps`, `asm-first-steps/sum-array`. Files: `challenge.md`, `challenge-starter.*`, `challenge-solution.*`, and `challenge-tests.*` or `challenge-cases.json`.
> - **Weekly rhythm** is computed from `passedAt` timestamps (`src/lib/rhythm.ts`); only `users.weeklyGoal` and `users.timezone` are stored. Paced (XP-withheld) passes don't count.
> - **Personal bests:** `bestMs` isn't tracked (no timing data). `attemptsAtPass` gives "passed on the first run".
> - `fifty-runs-unaided` became `unaided-25` ("Own Compass").
> - Reviews only advance when due; failed runs never reset the schedule, only the "I need a refresher" button does.
> - Rebuild or backfill with `npm run recompute-xp` (ledger and review schedule) and `npm run backfill-achievements`.

A plan for taking Baremetal's gamification further. Read `CLAUDE.md`, `AGENTS.md` and
`HANDOFF.md` first. **Finish HANDOFF's "Next steps"** (browser check, e2e re-run, legal updates,
commit) before starting, so this work lands on a clean, committed tree.

## What already exists

| Piece | Where | Notes |
|---|---|---|
| Difficulty and XP | `src/lib/gamification.ts` | easy 5, medium 10, hard 20, impossible 40. 29 lessons = 370 XP. |
| Levels and titles | `levelFor()` | Level L needs 4·L·(L−1) XP, capped at 10 (Novice → Archmage). Finishing every lesson reaches exactly level 10. |
| XP award | `recordAttempt()` in `src/lib/progress.ts` | Server-side, once per lesson, race-safe. `/api/run` returns `reward` and the workspace shows "+XP / Level up". |
| XP rebuild | `scripts/recompute-xp.mts` | XP must always be derivable from stored data. **Keep this true.** |
| Leaderboard, profiles | `/leaderboard`, `/u/[username]`, `src/lib/community.ts` | All-time XP only. Opt-out with `profilePublic`. |
| Hints, tutor | `workspace.tsx`, `tutor-chat.tsx` | Hints are free, tracked only client-side (`hintsShown`) and in analytics (`hint_open`). |
| Dashboard | `src/app/dashboard/page.tsx` | Level card, lessons done, test runs, "up next", course progress. |

## Principles

These shape every feature below. If a feature conflicts with one, the principle wins.

1. **Reward learning, not grinding.** Points come from passing tests and meeting challenges,
   never from page views, time spent or repeated runs.
2. **Never punish asking for help.** Hints and the tutor stay free. No XP is lost or reduced for
   using them. "Without hints" can earn *extra* recognition, but never costs anything.
3. **Celebrate perseverance as much as speed.** A beginner who passes on attempt 14 has done
   something worth marking.
4. **No anxiety mechanics.** No loss-framed streaks, guilt notifications, countdown timers on
   lessons, or "you'll lose your progress" messages. Rhythm over pressure.
5. **The server is the source of truth.** Anything that gives XP or rank is computed server-side
   from stored data. Client-reported signals can only unlock cosmetic things.
6. **XP stays rebuildable.** Every XP source is recorded so `recompute-xp` can rebuild totals
   from scratch.
7. **Privacy by default.** New public data (badges, streaks) only appears when `profilePublic` is
   on. It's listed in the privacy notice, included in data export and removed on account deletion.
8. **Follow the house style.** Tokens only, WCAG AA, medieval titles that are never demeaning,
   no left-stripe callouts, `prefers-reduced-motion` respected, one `<h1>` per page, and
   request-time data inside `<Suspense>`.

---

## Phase A: foundations (do first)

Groundwork the other features need. There's little visible change, but skipping it means rework later.

### A1. XP ledger

Today XP lives in `users.xp` plus `progress.xpAwarded`. Challenges and seasons (below) need more
than one XP source and timestamps, so add a ledger.

- New collection `xp_events`: `{ userId, source: "lesson" | "challenge", ref: string, xp, at }`,
  with a unique index on `(userId, source, ref)`. The unique index *is* the "award once"
  guarantee.
- `recordAttempt` inserts a ledger row on first pass (duplicate-key error = already awarded),
  then `$inc`s `users.xp` as now. Keep `progress.xpAwarded` for display.
- `recompute-xp` sums the ledger. Add a one-off backfill: one `lesson` row per passed lesson,
  using `passedAt`.
- Export and delete in settings cover `xp_events`.

### A2. Record hint use on the server

The "without hints" features need it, and today it's client-only.

- `POST /api/progress` (exists) gains `{ exerciseId, event: "hint", index }` and sets
  `progress.hintsUsed = max(hintsUsed, index + 1)`. Signed-in users only, rate-limited like the
  other routes.
- Reload-safe: `hintsShown` should start from `progress.hintsUsed` when the page loads.
- On first pass, snapshot `hintsUsedAtPass` and `attemptsAtPass` onto the progress doc.

### A3. Learner timezone

Weekly rhythm and seasons need the learner's calendar.

- Store `users.timezone` (IANA name from `Intl.DateTimeFormat().resolvedOptions().timeZone`),
  sent at sign-up and silently refreshed at login. It's editable in settings. Default to `UTC`.

---

## Phase B: core loop

### B1. Lesson mastery: three seals

Each lesson shows up to three seals (medieval wax seals, drawn as simple shapes, not emoji):

| Seal | Earned by | XP |
|---|---|---|
| **Passed** | Passing the tests | Lesson XP (unchanged) |
| **Unaided** | Passing with `hintsUsedAtPass === 0`, no tutor messages before the pass, and at least one failed run or hard/impossible difficulty | None (cosmetic) |
| **Challenge** | Passing the lesson's optional challenge (B2) | Challenge XP |

- **Unaided** is cosmetic because it's partly client-observable. It also avoids paying people to
  avoid help.
- **Unaided** can't be earned on an easy lesson passed first try: reading the starter shouldn't
  count as "unaided" effort, and the requirement keeps the seal meaningful.
- Seals appear on track lists, the lesson header and the public profile.
- The tutor-message check uses `tutor_threads` (messages carry `at`).

### B2. Lesson challenges (side quests)

Optional harder variants of existing lessons. Content work is the bulk of this.

- New optional files per lesson: `challenge.md` (the brief) and `challenge-tests.c|rs` (hidden
  tests), or stdout cases in `challenge-cases.json`. The loader treats them as optional.
- Examples:
  - *Reverse a string* → in place, with no second buffer
  - *Linked lists* → also reverse the list
  - *HTTP server* → handle keep-alive
- Unlocks after the lesson is passed. It uses the same editor, with a "Challenge" toggle in the
  workspace header.
- XP: half the lesson's XP (easy 3, medium 5, hard 10, impossible 20). Ledger
  `source: "challenge"`, `ref: exerciseId`.
- `verify-curriculum` checks that the reference challenge solution passes and that the plain
  lesson solution **fails** the challenge tests. Otherwise the challenge isn't a challenge.
- Start with **one challenge per track** (4 total), then expand once the format is proven.

**Level curve consequence:** challenges add XP beyond 370, so "finishing everything = level 10"
stops being true. Decide one of:

- (a) Keep level 10 as the cap and let XP keep growing past it. This is simplest.
- (b) Extend the curve and add titles beyond Archmage.

**Recommendation: (a)** for now. The cap message changes to
"Maximum level. Challenges still earn XP for the leaderboard."

### B3. Achievements: the Codex

A small, curated set of named achievements, shown on the dashboard and profile. These are
server-computed from progress, ledger and timestamps, stored in `achievements` with
`{ userId, id, unlockedAt }` and a unique index. They're evaluated in `/api/run` after a pass
(cheap set lookups) and by a backfill script for existing users. **No XP.** Achievements are
recognition, so the leaderboard stays about learning.

Starter set (about 15). Names fit the medieval register and are never demeaning:

| Id | Name | Condition |
|---|---|---|
| `first-pass` | First Light | Pass any lesson |
| `first-try` | Clean Strike | Pass a medium-or-harder lesson on the first run |
| `persevere` | Siege Breaker | Pass a lesson after 10 or more failed runs |
| `unaided-5` | Self-Taught | Earn 5 Unaided seals |
| `track-first-steps` | First Steps Taken | Finish *First Steps* |
| `track-c-memory` | Keeper of Pointers | Finish *C Memory* |
| `track-rust-foundations` | Friend of the Borrow Checker | Finish *Rust Foundations* |
| `track-rust-http` | Gatekeeper | Finish *Rust HTTP* |
| `polyglot` | Two Tongues | Pass a C lesson and a Rust lesson |
| `impossible` | The Impossible Made Possible | Pass an impossible lesson |
| `all-impossible` | Legend | Pass every impossible lesson |
| `challenge-1` | Side Quest | Pass any challenge |
| `challenge-track` | Completionist | All challenges in one track |
| `weekly-4` | Steady Hand | Hit your weekly goal 4 weeks running (B4) |
| `archmage` | Archmage | Reach level 10 |

- Define them in code (`src/lib/achievements.ts`) as `{ id, name, description, check(ctx) }`,
  so adding one is a single entry plus a backfill run.
- Locked achievements show their name and condition (dimmed), so learners can aim at them. None
  are secret except, optionally, `persevere`, which is a nicer surprise.

### B4. Weekly rhythm instead of daily streaks

Daily streaks drive anxiety and punish life getting in the way. Use a weekly goal instead.

- In settings, the learner picks a goal of **1, 2, 3 or 5 lessons a week** (default 2), or
  "No goal".
- The week runs Monday to Sunday in the learner's timezone. Progress counts lessons first-passed
  plus challenges passed.
- **Rhythm** is the count of consecutive weeks where the goal was met. If a week is missed, the
  rhythm quietly restarts with no "you lost your streak" messaging. The best rhythm is kept and
  shown.
- One **rest week** is banked automatically every 4 weeks met (maximum 2), and is used
  automatically on a missed week. It's mentioned only in a tooltip.
- Show it on the dashboard as a small ring ("2 of 3 this week") plus the rhythm count.
- No emails or push notifications for any of this.

---

## Phase C: moments and visibility

### C1. Celebration polish

- **Level up:** a modal (focus-trapped, `aria-modal`) shows the new title and its short
  description. Add a one-line flavour description per title in `gamification.ts`, for example
  Squire: *"You can hold your own with loops and functions."*
- **Track complete:** a dedicated moment after the last lesson of a track. Show the
  achievement, a summary of seals earned, and a link to the next track.
- **Achievement toast:** a small toast in the corner via the existing `role="status"` pattern.
  Multiple unlocks queue up.
- **Motion:** at most a short scale and fade. Confetti only behind `prefers-reduced-motion: no-preference`
  and only for track completion and level 10. No sound.

### C2. Course map

Replace or augment the dashboard's course list with a map: each track is a path of lesson nodes,
with a node's state shown as locked, available, passed or with seals. Use CSS and SVG, not a canvas.
Keyboard-navigable (nodes are links in order), with a list view fallback below `sm`.
Prerequisites (`track.prerequisite`) appear as a path joining two tracks.

### C3. Profile showcase

`/u/[username]` gains:
- seals per track
- unlocked achievements, newest first
- best weekly rhythm
- level title with its flavour line

The current weekly progress stays private.

---

## Phase D: solo-first value, optional community

Phases A to C already give a single learner plenty to do. Phase D adds what keeps one person
coming back with **zero other users**, then adds social features that stay hidden until there
are enough people to make them worthwhile. Nothing in D may depend on other users existing.

**Already implemented** (so D doesn't rebuild it): XP, levels and titles (`levelFor`), the
dashboard's level card, lessons done, test runs, "up next" and course progress, the all-time
`/leaderboard` (`getLeaderboard`, top 50 by XP, `profilePublic` opt-out), username prefix search
(`searchUsers`) and `/u/[username]` (`getPublicProfile`). **Not implemented:** streaks or
rhythm, personal bests, review of past lessons, daily suggestion, any per-day activity data.
The leaderboard currently renders whatever rows exist, including a list of one or none.

### D1. Review queue (spaced repetition of past lessons)

The most useful solo feature: a learner who has finished everything still has a reason to return.

- New collection `reviews`: `{ userId, exerciseId, dueAt, interval, lastAt }`, unique on
  `(userId, exerciseId)`. A first pass creates a row with `dueAt` = +3 days.
- Review means re-solving the lesson from a fresh starter. Pass: `interval` goes 3, 7, 21, 60
  days. Fail or "I need a refresher": back to 3. Review gives **no XP** (principle 1), so there
  is nothing to farm.
- The dashboard shows "N lessons ready for review" with a link to the oldest. No overdue
  counters, no red badges (principle 4). With nothing due the card is simply absent.
- Reuses the existing editor and `/api/run`; the only server change is a `review` flag so
  `recordAttempt` updates the schedule instead of awarding XP.

### D2. Personal stats and bests

All derived from data already stored or added in A and B, shown on the dashboard and **private**.

- Per-lesson personal bests: fewest attempts to pass, and (once recorded) fastest pass time from
  first run, kept in `progress` (`bestAttempts`, `bestMs`).
- Totals: lessons passed per track, XP by month (from the `xp_events` ledger), seals earned,
  hint use trend. Plain tables or a small SVG bar chart; no comparison to anyone else.
- A "Your week" strip using the B4 weekly rhythm, so the rhythm is visible without needing
  friends.

### D3. Daily suggestion

One suggested lesson a day, chosen deterministically on the server so it needs no job runner.

- Priority: a review that is due, otherwise the next unpassed lesson in the learner's furthest
  track, otherwise an unpassed challenge, otherwise a random passed lesson to revisit.
- Shown as "Today's pick" on the dashboard. It is a suggestion only: skipping it costs nothing
  and nothing is tracked about whether it was followed.

### D4. Milestone achievements for own actions

Extend the B3 Codex with solo, effort-based entries (still no XP, still defined in
`src/lib/achievements.ts`): `review-1` (first review passed), `review-10`, `rhythm-8` (8 weeks
of rhythm), `revisit-track` (review every lesson in a track once), `fifty-runs-unaided`.
All are earned by the learner's own actions and need no other users.

### D5. Optional community, gated by user count

Social features are built last and ship **dormant**. A single helper in `src/lib/community.ts`,
`communityReady()`, returns true only when public learners (`profilePublic !== false` with a
username) with at least one pass number **20 or more** (constant `COMMUNITY_MIN_USERS`, env
override), cached for 10 minutes. Features below check it; when false they are hidden, not
shown empty.

- **Leaderboard:** below the threshold `/leaderboard` is not linked from the nav or sitemap and
  the page shows a short "Community opens when more learners join" note instead of a table
  (no rows, no "1st of 1"). Above it the page works as today.
- **Seasons:** only after the threshold, add **This month** and **All time** tabs from the
  ledger (`xp_events` filtered by `at`, index `(at, userId)`, cached 5 minutes with
  `"use cache"` / `cacheLife`; check the Next 16 docs). The tab is hidden if fewer than 5
  learners earned XP this month. Show "your position" even outside the top 50.
- **Profile extras:** a learner's own `/u/[username]` stays fully useful with nobody watching
  (seals, achievements, rhythm from C3). Rank is shown on it only when `communityReady()`.
- Season Laurels (top 3 monthly achievement) are dropped unless the owner wants them later.
- Suggested rollout: ship D1 to D4 first. Build D5 only after the first real cohort, and
  decide the threshold from actual numbers.

### D6. Out of scope for now

These need moderation, trust and safety work, or many more users: friends and following,
duels and head-to-head races, comments on profiles, XP gifting, paid cosmetics.

---

## Data model summary

| Collection / field | New | Purpose |
|---|---|---|
| `xp_events` | yes | XP ledger: unique `(userId, source, ref)`, index `(at, userId)` |
| `achievements` | yes | Unique `(userId, id)` |
| `reviews` | yes | Spaced-repetition schedule: unique `(userId, exerciseId)` |
| `progress.bestAttempts`, `bestMs` | yes | Personal bests (private) |
| `progress.hintsUsed`, `hintsUsedAtPass`, `attemptsAtPass` | yes | Seals and achievements |
| `progress.challenge` | yes | `{ status, attempts, passedAt }` for the lesson's challenge |
| `users.timezone`, `weeklyGoal`, `rhythm { current, best, restWeeks, lastWeek }` | yes | Weekly rhythm |

Each needs to be covered by settings' **data export** and **account deletion**, and listed in
`content/legal/privacy.md`, which notes what appears publicly.

## Order of work

1. Clear the HANDOFF next steps and commit.
2. A1 ledger and backfill, then update `recompute-xp`, export and delete. Run the e2e suite.
3. A2 server-side hints, A3 timezone.
4. B3 achievements (immediate value, no content needed) and C1 toasts and level-up modal.
5. B1 seals.
6. B4 weekly rhythm.
7. B2 challenges, one per track, plus `verify-curriculum` support.
8. C2 course map, C3 profile showcase.
9. D1 review queue, D2 personal stats, D3 daily suggestion, D4 solo achievements.
10. D5 community features, only once there is a real cohort; check `communityReady()` gates before shipping.

Each step should leave typecheck, lint, `next build`, `verify:content` and `test:e2e` green,
and be committed separately.

## Open decisions for the owner

1. Level curve once challenges exist: cap at 10 (recommended) or add titles?
2. Should the Unaided seal require no tutor use as well as no hints? (The plan says yes.)
3. Default weekly goal: 2 lessons?
4. Community threshold: 20 public learners, or a different number? Season Laurels: yes or no?
5. Should achievement names be shown on public profiles by default, or only when opted in?
