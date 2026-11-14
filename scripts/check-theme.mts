// Theme guard (see CLAUDE.md "Theme"): no hard-coded hex colours and no left accent stripes
// in src/components and src/app. Colours must come from tokens in globals.css.
import { readdirSync, readFileSync } from "node:fs";
import { join, relative } from "node:path";
import { THEME_COLORS } from "../src/lib/theme-colors";

const repo = join(import.meta.dirname, "..");

// Files exempt for good reason.
const EXEMPT = [
  /(^|\/)globals\.css$/, // token definitions
  /opengraph-image\.tsx$/, /twitter-image\.tsx$/, // image rendering has no CSS variables
  /(^|\/)icon\.[a-z]+$/, /apple-icon\.[a-z]+$/, /favicon\.ico$/, // icons
];

// Existing violations, allowed until fixed. Keep this list shrinking.
// Currently empty. manifest.ts, the layout.tsx viewport and global-error.tsx need literal values
// (no CSS vars, or no stylesheet), so they import src/lib/theme-colors.ts, which is checked
// against globals.css below.
const ALLOW = new Set<string>();

const HEX = /(?<![\w&/-])#(?:[0-9a-fA-F]{8}|[0-9a-fA-F]{6}|[0-9a-fA-F]{4}|[0-9a-fA-F]{3})\b/;
// border-l / border-l-4 / border-l-[3px] / border-s-* / inline borderLeft / css border-left
const STRIPE = /(?<![\w-])border-(?:l|s)(?:-[\w[\]#./()-]+)?(?![\w-])|\bborderLeft(?:Width|Color)?\b|border-left\b/;

function* walk(dir: string): Generator<string> {
  for (const e of readdirSync(dir, { withFileTypes: true })) {
    const p = join(dir, e.name);
    if (e.isDirectory()) yield* walk(p);
    else if (/\.(tsx?|css|mdx?)$/.test(e.name)) yield p;
  }
}

let bad = 0;
let allowed = 0;

// theme-colors.ts must mirror the tokens in globals.css (default dark block, then light block).
const css = readFileSync(join(repo, "src/app/globals.css"), "utf8");
const token = (block: string, name: string) => new RegExp(`--${name}:\\s*(#[0-9a-fA-F]{6})`).exec(block)?.[1]?.toLowerCase();
const darkBlock = css.slice(css.indexOf(":root {"), css.indexOf("@media (prefers-color-scheme: light)"));
const lightBlock = css.slice(css.indexOf(':root[data-theme="light"]'));
const mirrored: [string, string, string | undefined][] = [
  ["dark.bg", THEME_COLORS.dark.bg, token(darkBlock, "bg")],
  ["dark.text", THEME_COLORS.dark.text, token(darkBlock, "text")],
  ["dark.muted", THEME_COLORS.dark.muted, token(darkBlock, "muted")],
  ["dark.accent", THEME_COLORS.dark.accent, token(darkBlock, "accent")],
  ["dark.accentInk", THEME_COLORS.dark.accentInk, token(darkBlock, "accent-ink")],
  ["light.bg", THEME_COLORS.light.bg, token(lightBlock, "bg")],
];
for (const [name, got, want] of mirrored) {
  if (got.toLowerCase() !== want) {
    bad++;
    console.error(`src/lib/theme-colors.ts: ${name} is ${got} but the globals.css token is ${want ?? "missing"}`);
  }
}
for (const base of ["src/components", "src/app"]) {
  for (const file of walk(join(repo, base))) {
    const rel = relative(repo, file);
    if (EXEMPT.some((r) => r.test(rel))) continue;
    readFileSync(file, "utf8").split("\n").forEach((line, i) => {
      const hex = HEX.test(line);
      const stripe = STRIPE.test(line) && !/cm-cursor/.test(line); // editor caret is not an accent stripe
      if (!hex && !stripe) return;
      if (ALLOW.has(rel)) { allowed++; return; }
      bad++;
      console.error(`${rel}:${i + 1}: ${hex ? "hard-coded hex colour" : "left accent stripe"}: ${line.trim().slice(0, 120)}`);
    });
  }
}
if (bad) {
  console.error(`\n${bad} theme violation(s). Use tokens from globals.css; cards use a full border and a tint.`);
  process.exit(1);
}
console.log(`theme ok (${allowed} allowlisted line(s) in ${ALLOW.size} files)`);
