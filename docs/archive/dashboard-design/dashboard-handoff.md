# Baremetal dashboard redesign: handoff

Reference mockup: `dashboard-reference.html` (in this folder) is the design source (a template file with `{{placeholders}}` and a small logic block; it does not render on its own, read it for exact markup, inline styles and values). Re-implement it in the app's existing stack and components. Keep the app's real data and routing; the values in the mockup are sample data copied from the current dashboard.

## What changes
Dashboard page only (`/`, the logged-in home). Same content as today: resume lesson, level/XP, lessons passed, courses completed, not-started courses.

## Tokens

Colors
- ground `#0c0d0f`
- panel `#131417`, panel hover/row hover `#181a1e`, nav active `#17181c`
- border `#23252a`, strong border (tags, ghost buttons, todo nodes, connectors) `#2f3238`
- text `#ecebe8`, secondary `#b3b4ba`, muted `#9a9ca3`, todo-number `#8b8d94`
- accent `#f5a524` (text on accent: `#0c0d0f`)

Accent means progress and action only: primary button, completed lessons and the connector up to the current lesson, current-lesson ring, XP bar fill, "Level 3" label, lessons-passed number, "Recommended next" text and its language tag, active-nav underline, avatar, hover state on course rows. Do not use it for anything decorative. No gradients, glows or glass.

Type
- Display: Archivo, `font-stretch: 118%` (112% for course titles), weight 700-750, letter-spacing -0.02 to -0.03em. Used for the lesson title (34px), "Squire" (38px), section heading (21px), stat numbers (24px), course titles (17px), wordmark (17px).
- Body: IBM Plex Sans 400/500/600, 15px base, `font-variant-numeric: tabular-nums` on the whole page.
- Mono: IBM Plex Mono 500, 13px, language tags only.
- Load Archivo with the width axis: `family=Archivo:wdth,wght@62..125,100..900`.

Shape and spacing
- Panels: 16px radius, 1px `#23252a` border, `inset 0 1px 0 rgba(255,255,255,0.04)`, no drop shadows.
- Buttons/tags/ghost buttons 6-8px radius, lesson nodes full circle, XP bar 2px radius.
- Page max-width 1200px, 32px side padding, 24px gap between panels.
- Tap targets at least 44px (primary button 48px, account button 44px, rows 44px+).

## Layout
1. Header: wordmark with logo mark, nav (Dashboard active, Courses, Leaderboard), account button (avatar initial + name).
2. Row 1, wraps to a stack on narrow screens: resume panel (flexible, min 600px) beside a level panel (min 320px).
   - Resume panel: breadcrumb line (course / lesson n of total / language, level), lesson title, description (max 560px), then one row containing the 8-step lesson path and the "Open lesson 15 min" button.
   - Lesson path: numbered circles joined by lines. Done = solid accent with check; current = accent ring with number; todo = outlined, muted. The connector is accent up to the current lesson.
   - Level panel: "Level 3" (accent), level name, XP bar, "30 / 48 XP" and "18 XP to level 4", then two rows: lessons passed and courses completed.
3. Row 2: one panel titled "Not started" with a count (accent) and an "All courses" link. Each course is a full-row link: language tag (48x44), title, note line, level, lesson count, hours, and a "View" button that is always visible. The recommended course has an accent-filled tag and an accent "Recommended next · Builds on X" note.

## Behaviour
- Rows are links to the course page. Hover: row background, "View" and tag border turn accent.
- Visible focus ring on every interactive element (2px `#ecebe8`, 3px offset).
- Lesson nodes need accessible labels ("Lesson 3, complete" / "up next" / "not started").
- No motion beyond a 1px press on the primary button.

## Data to wire up (mockup values are samples)
resume lesson (title, description, minutes, index/total, course name/language/level), level (number, name, xp, xp for next level), totals (lessons passed/total, courses completed/total), not-started courses (title, language, level, lesson count, hours, builds-on, recommended flag). The "Systems Programming in C" row in the mockup has placeholder values for level and note; use the real fields.
