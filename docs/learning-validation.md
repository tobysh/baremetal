# Learning experience validation

Implemented checks run with `npm run check:learning`, `npm run test:runner` and
`npm run test:regressions` (the last command builds its own production snapshot and requires MongoDB).
Browser artifacts are copied to a unique subdirectory of `test-results/` before the temporary app is removed. To use an installed Chromium
locally, set `PLAYWRIGHT_CHROMIUM_EXECUTABLE=/usr/bin/brave`; otherwise install the
Playwright browser with `npx playwright install chromium`.

The regression harness copies source into a temporary directory, builds there without touching the shared `.next` output, creates its own database and secrets, starts the web app
and runner on a temporary socket, disables external tutor/email delivery, and
cleans up processes and data. Do not point the browser suite directly at a live
site; it only accepts the harness's scratch database names.

## Observe five beginners

This needs real learners and has not been performed by automated checks. Ask each
person to start with Programming from Zero, complete the opening lessons and
continue into C Data. Let them choose their own next action before prompting.
Record only observations with their agreement, avoiding names, source code and
account details in shared notes.

| Participant | Where they hesitated | Expected next action | Observed next action | Instruction to improve |
| --- | --- | --- | --- | --- |
| 1 | Pending | | | |
| 2 | Pending | | | |
| 3 | Pending | | | |
| 4 | Pending | | | |
| 5 | Pending | | | |

Prioritise blocked progress first, repeated confusion second, then wording polish.
Use the cohort report to check whether the affected lesson improves over time;
small samples and different cohorts do not establish causation.

## Manual accessibility pass

Check desktop and 375px layouts with keyboard only and a screen reader. Confirm
editor entry and Escape-then-Tab exit, tab selection, hints, error announcements,
save retries, celebration dialog focus containment/return, and reduced motion.
Browser regressions cover keyboard behaviour and viewport overflow; they do not
replace a screen-reader session. Both the screen-reader session and beginner
observations remain pending until recorded here.

## Cohort definitions

The admin stuck report uses existing account progress only. Cohorts start on
Monday UTC. A learner/course with any missing or invalid start date is excluded
from time-based cohorts, rather than guessing an earlier start. Groups below five
learners are hidden. Completion is cumulative, not a fixed-window success rate.
Median first-pass time includes only valid recorded first passes after course start.

Later learning activity means a recorded start or first pass within that same
course from day 7 inclusive through day 14 exclusive. The report withholds the
rate until every possible start in the cohort week has had its observation window.
Repeated edits, review visits, logins and work in other courses are not captured;
this is a lower bound on course learning activity, not retention. No visitor
identifiers, activity events or analytics-consent changes were added.
