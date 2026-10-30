# UX polish plan (30 October 2026)

Sixteen polish items, split into five workstreams. Each stream owns a disjoint set of files so they can run in parallel in one working tree.

## Rules for every stream

- Follow `CLAUDE.md`: colour tokens only (dark and light), no left accent stripes, WCAG AA in both themes, one `<h1>` per page, lazy-load heavy client code, request-time data inside `<Suspense>`.
- Respect `prefers-reduced-motion`. Every new interactive element needs a keyboard path and an accessible name.
- Do not commit, do not run `recompute-xp`, do not edit `content/legal/` or `src/lib/site.ts`. Report any new cookie or localStorage key instead, so legal pages can be updated once.
- Only edit files you own. Type errors in files owned by other streams are expected while work is in flight; ignore them.
- Finish by running `npx tsc --noEmit` and `npm run lint` and fixing problems in your own files.

## Streams

### A. Workspace (owns `src/components/workspace/*`)
- 1. After a pass, the primary button becomes "Next lesson →" with a keyboard shortcut.
- 3. Inline ⌘/Ctrl+Enter hints next to Run and Test (not only `title`), plus a dismissible one-time first-lessons tour of Run, Test, Hints and the tutor.
- 5. Autosave indicator ("Saving…" / "Saved · 2s ago").
- 7. Run output: Compiling… then Running…, elapsed time, copy-output button, scroll to the first failing test.
- 13. Editor font-size and tab-width controls (persisted per viewer) and a visible reset with undo.

### B. Dashboard and feedback (owns `src/components/celebrations.tsx`, `dashboard-sections.tsx`, `gamification.tsx`, `seals.tsx`, `src/app/dashboard/*`, `src/app/leaderboard/*`, `src/app/u/*`)
- 1. "Continue where you left off" card on the dashboard.
- 8. Celebration dialogs: single-key dismiss, a "Next lesson" button, queueing so rapid passes do not stack.
- 9. Empty states for dashboard, leaderboard, achievements.
- 11. Screen-reader pass: dialogs and toasts announce correctly, test pass/fail announced via live regions.

### C. App chrome (owns `src/components/site-header.tsx`, `header-navigation.tsx`, `nav-progress.tsx`, `theme-toggle.tsx`, `src/app/layout.tsx`, `src/app/template.tsx`, new `src/components/command-palette*`)
- 2. ⌘K command palette (lazy-loaded) for lessons, tracks, settings, shell.
- 10. Skip-to-content link.
- 14. Weekly rhythm dots in the header, quiet and non-nagging.
- 16. Short page transition in `template.tsx`, reduced-motion safe. Put CSS in a CSS module, not `globals.css`.

### D. Pages and global CSS (owns `src/app/globals.css`, `src/app/page.tsx`, new `loading.tsx` files, `src/components/skeletons.tsx`, `src/components/ui.tsx`)
- 6. `loading.tsx` with skeletons for `/leaderboard`, `/u/[username]`, `/settings`, `/shell`, `/learn`.
- 12. Contrast audit of muted and accent text in both themes; fix token values where below AA.
- 15. Landing page: looping write → Run → pass demo (CSS or SVG, reduced-motion safe). The "try without signing up" path is not built: it needs an owner decision on anonymous sandbox quota.

### E. Responsive audit (read-only)
- 4. Playwright pass at 390px and 1440px over landing, dashboard, track, lesson, shell, settings. Report concrete layout bugs with file and line suggestions. No edits.

## After the streams finish

1. Collect new storage keys and update `content/legal/*.md`, bump `SITE.legal.lastUpdated`.
2. `npx next typegen`, `npx tsc --noEmit`, `npm run lint`, `npm run build`.
3. Apply fixes from the stream E report.
4. Manual click-through, then commit.

## Draft commit message

```
Polish UX: next-lesson flow, command palette, run feedback, a11y

Workspace: next-lesson button after a pass, inline shortcut hints and a
first-lessons tour, autosave indicator, compile/run phases with timing and
copy-output, editor font and tab-width controls.

Dashboard: continue-where-you-left-off card, queued and keyboard-dismissible
celebrations, empty states, live-region announcements for results and toasts.

Chrome: Cmd/Ctrl+K command palette, skip link, weekly rhythm dots in the
header, reduced-motion-safe page transitions.

Pages: loading skeletons for leaderboard, profile, settings, shell and learn;
light/dark contrast fixes; landing page demo.
```
