// Theme guard (see CLAUDE.md "Theme"): no hard-coded hex colours and no left accent stripes
// in src/components and src/app. Colours must come from tokens in globals.css.
import { readdirSync, readFileSync } from "node:fs";
import { join, relative } from "node:path";

const repo = join(import.meta.dirname, "..");

// Files exempt for good reason.
const EXEMPT = [
  /(^|\/)globals\.css$/, // token definitions
  /opengraph-image\.tsx$/, /twitter-image\.tsx$/, // image rendering has no CSS variables
  /(^|\/)icon\.[a-z]+$/, /apple-icon\.[a-z]+$/, /favicon\.ico$/, // icons
];

// Existing violations, allowed until fixed. Keep this list shrinking.
// TODO: manifest.ts and the layout.tsx viewport themeColor need literal values (not CSS vars);
//       keep them in sync with the --bg tokens, or derive them from one shared constant.
// TODO: global-error.tsx renders outside the root layout, so no stylesheet is loaded; inline hex
//       is needed unless it is rewritten to use a tiny inline <style> that mirrors the tokens.
const ALLOW = new Set(["src/app/manifest.ts", "src/app/layout.tsx", "src/app/global-error.tsx"]);

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
console.log(`theme ok (${allowed} allowlisted line(s) in ${ALLOW.size} files, see TODOs in scripts/check-theme.mts)`);
