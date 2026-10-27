# Baremetal — project rules

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
