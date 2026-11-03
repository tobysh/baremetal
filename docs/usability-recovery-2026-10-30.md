# Usability recovery — 30 October 2026

Interrupted curriculum and UX work was reviewed without committing, deploying, changing learner progress or recomputing XP.

## Restored behavior

The catalogue previously failed to load because incomplete new courses were discovered as published content. Loading any exercise could fail on a missing file in an unrelated course. Explicit draft flags now preserve unfinished work while excluding it from the catalogue, search, recommendations, static lesson routes and exercise lookup.

- Set the boolean `"draft": true` in a course's `track.json` to draft that whole course.
- Set `draft: true` in a lesson's frontmatter to draft a lesson in an otherwise published course.
- Missing files in published content remain errors. Nothing is silently skipped merely because it is broken.
- For draft validation, use an isolated working copy with its flags cleared. Remove the original draft flags only when content is complete and its starters, solutions and tests have been verified. Check prerequisites before publishing a course.

Twelve unfinished courses remain on disk as drafts:

| Area | Course directories |
| --- | --- |
| C | `c-allocators`, `c-embedded`, `c-performance` |
| Go | `go-distributed`, `go-fault-injection`, `go-maintenance`, `go-protocols`, `go-query-execution` |
| Rust | `rust-ffi-boundaries` |
| Security | `sec-incident-response`, `sec-reverse-engineering`, `sec-secure-delivery` |

The unpublished Toy Malware Development extensions `08-relocations-and-symbols`, `09-embedded-configuration`, `11-virtual-scheduling` and `12-crash-recovery-teardown` are also drafts. Their files were preserved. Existing published material, including the completed debugging course and the seven-lesson toy malware track, remains available. Five missing published debugging lesson IDs were added to the stable-ID snapshot; unfinished draft IDs were not added.

The published catalogue contains 39 courses and 491 lessons at this recovery snapshot.

## UX fixes

- Celebration dismissal now restores focus to the current primary workspace action if passing replaced the original Test button with a continuation link.
- Workspace celebrations receive the next-lesson destination.
- Save feedback has a stable text element alongside its visual elapsed age.
- The desktop editor can shrink to leave room for output controls. The minimum output-panel height was reduced so the combined controls fit short desktop viewports instead of being clipped.
- Browser checks follow the updated progress labels and distinguish small control announcements from announcing full program output.
- The Cookie Notice describes the new font-size, tab-width and introductory-tour browser preferences. Its existing update date already matches this change's date.

## Regression coverage

`npm run check:curriculum` checks incomplete drafts, direct draft-course lookup, strict failure for incomplete published content and malformed track draft flags using temporary fixtures. This check was added to CI.

Browser checks include command-palette navigation and clickable output controls at a 1280×720 viewport with the editor split set to its maximum. Existing save-recovery, cancellation, account, accessibility and responsive checks remain in place.

## Validation

| Check | Result |
| --- | --- |
| Isolated production builds (webpack and default Turbopack) | Passed |
| Account, tutor, API and live Run/Test/autosave regressions | Passed |
| Playwright browser suite | 22 passed |
| Runner socket, all five execution modes, admission and cancellation | Passed |
| Shell web integration | All 13 groups passed |
| Shell broker and launcher simulations | Passed |
| Curriculum draft isolation | Passed |
| TypeScript, ESLint, theme and learning checks | Passed |
| Published lesson-ID preservation | Passed |
| Full published curriculum verification | All 491 lessons across 39 courses passed, including starters and optional challenges |

Production snapshots and browser tests ran in temporary application directories with unique scratch databases. The regression harness cleaned up its processes and scratch database. Browser artifacts are under ignored `test-results/bm_regression_79c8ddef4c926c5b/`.

## Remaining host-dependent work

Real microVM shell boot was not verified: this environment has no `/dev/kvm`, and guest kernel/rootfs paths were not provided. Shell API and simulated broker tests passing do not replace that host check. The existing shell remains optional; deployment requirements and outstanding host validation are documented in [shell-deployment.md](shell-deployment.md) and [interactive-shell-isolation.md](interactive-shell-isolation.md).

Hostile workload tests were not run on this shared development environment. No host services, firewall rules or VM images were installed as part of this recovery.

## Local use

Dependencies are installed and the application builds. Use `npm run db` if MongoDB is not already running, then `npm run dev` for the local app. The production path still requires the separate runner service and the deployment environment described in [deployment-hardening.md](deployment-hardening.md).
