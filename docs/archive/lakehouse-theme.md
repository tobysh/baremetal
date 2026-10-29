# Lakehouse theme for Baremetal

A brief for restyling Baremetal in the "Lakehouse" theme. The theme already exists on the
owner's desktop (i3, polybar, rofi, Ghostty, VSCodium); this document translates it to the web
app. **Change how the app looks, not how it works:** no layout, copy, routing or behaviour changes
unless a section below asks for one.

Before writing code, read the Next.js notes in `AGENTS.md`. This project uses a newer Next.js
than you may know, so check `node_modules/next/dist/docs/` for `next/font` usage.

---

## 1. The idea

The feeling of being inside a lakeside cabin in the evening. The lamps are on, the wood is dark
and warm, and through the window the lake has gone a still slate blue. Calm, unhurried, lived-in.
The palette comes from a photo of wooden rowing boats pulled up on a pebble shore at sunset.

Three rules carry the whole theme:

1. **Warm and dark almost everywhere.** Backgrounds are warm browns (cedar), text is warm
   off-white (linen). No pure white, no pure black, no cool greys.
2. **Colour means something.** Cool lake blue marks *where you are* (focus, selection, active nav).
   Warm lamplight marks *what to act on* (primary buttons, links, progress, the cursor).
   Ember appears only when something is wrong. Moss only for success.
3. **The lake and dock are hints, not props.** No anchors, boats, rope textures, wave
   illustrations or wood-grain images. The setting comes through colour, a thin "waterline"
   rule, an occasional `∼` separator and a little frosted glass. If an addition reads as
   nautical decoration, leave it out.

The light theme (the site follows the OS setting) is the **same house in daylight**: linen paper,
warm ink, the same accents, darkened enough to pass contrast.

---

## 2. Palette

### Source colours (desktop names)

| Name | Hex | Role on the desktop |
|---|---|---|
| Deep | `#1B1A18` | Outer edges, side panels |
| Cedar | `#22201E` | Main background |
| Shelf | `#2E2B28` | Raised surfaces, the selected item |
| Linen | `#DCD2C0` | Main text |
| Driftwood | `#9C8B76` | Quiet text, labels |
| Lake | `#7F9BB3` | Focus and selection accent |
| Mist | `#4A5560` | Hairlines, separators, the waterline |
| Lamplight | `#D9A35F` | Warm highlight, the clock, the cursor |
| Ember | `#B86A4B` | Alerts |
| Moss | `#8A9A6B` | Success (terminal green) |
| Dusk mauve | `#9C8296` | Secondary hue (code keywords) |
| Still-water teal | `#7AA3A0` | Secondary hue (code types) |

Some web tokens below are slightly lighter or darker than these source colours so that text
passes WCAG AA (4.5:1). Use the token values, not the source values, for text.

### Token mapping

The app already has a token system in `src/app/globals.css` (`:root` plus a
`prefers-color-scheme: light` override, exposed to Tailwind through `@theme inline`). **Keep every
existing token name** so no component has to change. Only the values change, plus two new tokens
(`--mist`, `--lake-soft`) and a serif font.

Replace the two `:root` blocks in `src/app/globals.css` with:

```css
/* Lakehouse: a lake house in the evening (dark, default) and in daylight (light). */
:root {
  --bg: #1b1a18;            /* deep: page background */
  --surface: #22201e;       /* cedar: cards, panels, header */
  --surface-2: #2e2b28;     /* shelf: raised/inset areas, inline code, table headers */
  --border: #3d3934;        /* worn-wood hairline */
  --mist: #4a5560;          /* the "waterline": header rule, separators */
  --text: #dcd2c0;          /* linen */
  --muted: #a3937e;         /* driftwood, lifted for AA on every surface */
  --accent: #d9a35f;        /* lamplight: primary actions, links, progress */
  --accent-ink: #1b1a18;    /* text on lamplight */
  --accent-soft: rgb(217 163 95 / 0.12);
  --ok: #9dae7b;            /* moss, lifted */
  --ok-soft: rgb(157 174 123 / 0.12);
  --err: #d07f5e;           /* ember, lifted */
  --err-soft: rgb(208 127 94 / 0.12);
  --focus: #7f9bb3;         /* lake: focus rings, selection, "you are here" */
  --lake-soft: rgb(127 155 179 / 0.16);
  color-scheme: dark;
}

@media (prefers-color-scheme: light) {
  :root {
    --bg: #f5efe4;          /* linen paper */
    --surface: #fcf9f3;
    --surface-2: #ede5d6;
    --border: #d9cdb9;
    --mist: #b9c3cc;
    --text: #2a2420;        /* warm ink */
    --muted: #675948;
    --accent: #8a581a;      /* lamplight, darkened for contrast */
    --accent-ink: #fcf9f3;
    --accent-soft: rgb(138 88 26 / 0.1);
    --ok: #4f6133;
    --ok-soft: rgb(79 97 51 / 0.1);
    --err: #9b4428;
    --err-soft: rgb(155 68 40 / 0.08);
    --focus: #3c6283;
    --lake-soft: rgb(60 98 131 / 0.12);
    color-scheme: light;
  }
}
```

In `@theme inline`, add the two new colours and the serif font:

```css
  --color-mist: var(--mist);
  --color-lake-soft: var(--lake-soft);
  --color-focus: var(--focus);
  --font-serif: var(--font-noto-serif), ui-serif, Georgia, serif;
```

Change `::selection` from `var(--accent-soft)` to `var(--lake-soft)`, because selection is a
"where you are" signal and that's lake's job.

### Verified contrast (WCAG 2.x)

Every text token passes 4.5:1 on all three surfaces.

| Token | Dark: bg / surface / surface-2 | Light: bg / surface / surface-2 |
|---|---|---|
| text | 11.6 / 10.8 / 9.4 | 13.4 / 14.6 / 12.2 |
| muted | 5.8 / 5.4 / 4.7 | 5.9 / 6.5 / 5.4 |
| accent | 7.8 / 7.2 / 6.3 | 5.3 / 5.7 / 4.8 |
| ok | 7.3 / 6.8 / 5.9 | 5.9 / 6.5 / 5.4 |
| err | 5.7 / 5.3 / 4.6 | 5.6 / 6.2 / 5.2 |
| focus | 6.0 / 5.6 / 4.9 | 5.6 / 6.1 / 5.1 |
| accent-ink on accent | 7.8 | 5.7 |

`--border` and `--mist` are decorative and are never used for text. If you add any new text and
background pairing, check it reaches 4.5:1 first.

---

## 3. Typography

The desktop uses Noto Serif with italic labels. That's what makes it feel like a cabin rather
than a dashboard. On the web, keep that character for reading and use sans for controls.

| Use | Font | Notes |
|---|---|---|
| Headings (h1–h3), page titles, hero/lede text | **Noto Serif** (new, `--font-serif`) | Weights 400/600, plus italic 400. Normal letter-spacing; don't tighten it. |
| Lesson and legal prose (`.prose`) | **Noto Serif** | Long-form reading is where serif earns its place. Keep `line-height: 1.7`. |
| UI chrome: nav, buttons, forms, badges, tables, settings | IBM Plex Sans (keep) | Controls stay crisp and familiar. |
| Code, terminal output, inline code | IBM Plex Mono (keep) | Unchanged. |

- Load it like the existing fonts in `src/app/layout.tsx`:
  `Noto_Serif({ variable: "--font-noto-serif", subsets: ["latin"], weight: ["400", "600"], style: ["normal", "italic"], display: "swap" })`,
  and add the variable to `<html>` alongside the Plex variables.
- **Italic muted labels for quiet information.** Metadata such as eyebrows, "lesson 3 of 12",
  timestamps, field hints and empty states goes in `font-serif italic text-muted`. Values and
  actions stay upright. You read the value first and the label second.
- Don't change copy or casing.

---

## 4. Shape, depth and the hints of lake

- **Radius:** cards, panels and code blocks 12px, buttons and inputs 8px, inline code 5px (as
  now). Nothing pill-shaped except small status badges.
- **Borders:** 1px `--border` on cards and inputs. Focused inputs get a `--focus` border plus the
  existing `:focus-visible` outline (already lake through `--focus`).
- **Shadows:** soft and warm, only on floating things (menus, dialogs, the tutor panel if it
  floats). Use something like `0 12px 32px -12px rgb(14 13 12 / 0.55)` in dark mode and
  `0 12px 32px -14px rgb(42 36 32 / 0.18)` in light mode. Cards on the page don't need shadows.
- **The waterline:** the site header gets a **2px bottom border in `--mist`**, the desktop
  bar's signature. Use it only there and as the `<hr>` colour in prose. Nowhere else.
- **Frosted header:** make the sticky header slightly translucent with a blur,
  `background: rgb(from var(--surface) r g b / 0.85); backdrop-filter: blur(12px);`,
  falling back to solid `--surface`. This mirrors the translucent terminal. Use it on the header
  only, never on content or the code editor.
- **The ripple separator:** where the UI shows a short inline list of metadata (footer links,
  "C · 12 lessons · ~3h"), you may use `∼` in `--mist` as the separator instead of `·`. Use it
  sparingly. Never put it in body text.
- **Active nav and tabs:** the current item gets `text-text` with a 2px **lake** (`--focus`)
  underline. Inactive items are `text-muted`. This is the desktop's current-workspace treatment.
- **Callouts:** follow the existing rule in `globals.css` (full border and tint, **never a left
  stripe**). Lakehouse doesn't change that.
- **Motion:** gentle. Fades and colour transitions at 150–200ms ease-out, no bouncy or springy
  movement. Keep the existing `prefers-reduced-motion` handling.

---

## 5. Colour roles by component

| Element | Treatment |
|---|---|
| Primary button | `bg-accent text-accent-ink`, hover slightly lighter (dark) or darker (light) |
| Secondary button | `bg-surface-2 text-text border-border`, hover border `--focus` |
| Destructive button | `bg-err text-accent-ink` (dark text on ember in dark mode 5.7:1, light text on ember in light mode 6.2:1) |
| Links in prose | `--accent` (lamplight) with an underline, as now |
| Focus ring, selection, active nav | `--focus` / `--lake-soft` (lake) |
| Progress bars, lesson completion | Fill `--accent`, track `--surface-2`. Completed checkmarks `--ok`. |
| Success / error messages | `ok-soft` / `err-soft` backgrounds with `--ok` / `--err` text and a 1px border in the same hue at about 30% |
| Tutor chat | Learner bubbles `--surface-2`. Tutor messages with no bubble, on `--surface`, prose in serif. The AI tutor should read like a calm note, not a chat app. |
| Skeleton shimmer | Unchanged. It already uses `--surface-2` and `--border`. |

---

## 6. Code editor (CodeMirror)

The editor stays dark in both site themes (as now). It becomes **cedar** and its syntax colours
come from the same palette as the owner's VSCodium and Ghostty. Replace the `C` object in
`src/components/workspace/code-editor.tsx`:

```ts
// Lakehouse: always dark (cedar), whatever the site theme. All text ≥ 4.5:1 on bg.
const C = {
  bg: "#22201e",        // cedar
  gutterBg: "#22201e",
  text: "#dcd2c0",      // linen          10.8:1
  muted: "#a0917d",     // driftwood       5.3:1 (4.8:1 on activeLine)
  activeLine: "#2a2724",
  selection: "#3a4450", // lake-tinted     text on it 6.6:1
  cursor: "#d9a35f",    // lamplight
  keyword: "#bfa3b8",   // dusk mauve      7.1:1
  type: "#8fbab6",      // still-water teal 7.6:1
  string: "#a9b987",    // moss            7.7:1
  number: "#e2b272",    // lamplight, lifted 8.4:1
  comment: "#9c8e7c",   // driftwood, italic 5.1:1
  macro: "#d99173",     // ember, lifted   6.4:1
  fn: "#9ab3c7",        // lake, lifted    7.5:1
};
```

Also change `.cm-matchingBracket` to `backgroundColor: "#3a4450", outline: "1px solid #7f9bb3"`.
Leave the rest of the editor alone.

---

## 7. Hard-coded colours to update

These files bypass the tokens and must be changed by hand:

| File | Change |
|---|---|
| `src/app/layout.tsx` | `themeColor`: dark `#1b1a18`, light `#f5efe4` |
| `src/app/manifest.ts` | `background_color` and `theme_color` `#1b1a18` |
| `src/app/global-error.tsx` | background `#1b1a18`, text `#dcd2c0`, muted `#a3937e`, button `#d9a35f` with text `#1b1a18`. Keep the system font here; it renders without the app's fonts. |
| `src/lib/og.tsx` | background `#1b1a18`, text `#dcd2c0`, eyebrow `#d9a35f`, subtitle and footer `#a3937e`, logo as below. Optionally add a 4px `#4a5560` waterline across the bottom. |
| `src/app/icon.svg` | Keep the shape. Rect fill `#d9a35f`, chevron and underscore strokes `#1b1a18`. |
| `src/app/apple-icon.png`, `src/app/favicon.ico` | Regenerate from the updated `icon.svg` at the same sizes. |

Afterwards, search for any remaining old values, for example
`grep -rniE '#(0e1013|15181d|1c2027|2a2f38|e8eaee|9aa3b2|f5a524|1a1203|3fcf8e|ff6b6b|7cb7ff|16191e)' src`.
The search should return nothing.

---

## 8. Don'ts

- No pure `#000` / `#fff`, and no cool neutral greys (the old `#0e1013`-style blacks).
- No nautical imagery, emoji or icons (⚓ ⛵ 🌊), and no wood or water textures or gradients.
- No neon, glow effects or saturated gradients. Everything is a little muted, like evening light.
- Don't use lamplight for "you are here" or lake for "click this". Keep the roles separate.
- Ember is for errors and destructive actions only, never decoration.
- Don't make content or the code editor translucent. Glass is for the header only.
- Don't add left-stripe callouts (this is an existing project rule).

---

## 9. Done when

- [ ] `globals.css` tokens, `@theme inline` additions and `::selection` are updated, and the Noto Serif font is loaded.
- [ ] Headings and `.prose` use the serif. UI controls stay Plex Sans and code stays Plex Mono.
- [ ] The header has the mist waterline and frosted background. Active nav uses the lake underline.
- [ ] The CodeMirror palette is replaced.
- [ ] Every file in section 7 is updated, and the old-colour search returns nothing.
- [ ] Both OS themes have been checked on: the landing page, `/learn`, a lesson with the editor and tutor, `/dashboard`, `/settings`, login and signup, an error state, and the 404 page.
- [ ] Any new text and background pairing reaches at least 4.5:1.
- [ ] `npm run lint` and `npm run build` pass.
