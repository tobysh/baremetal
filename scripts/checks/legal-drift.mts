// Stops the legal pages drifting from the code: every MongoDB collection used under src/ and every browser
// storage key / cookie must be covered by content/legal/*.md (or be listed here as holding no personal data).
// Usage: npm run check:legal. Update COLLECTIONS below when a collection or key is added, together with the page.
import { readdirSync, readFileSync, statSync } from "node:fs";
import path from "node:path";

const root = path.resolve(import.meta.dirname, "../..");
const legal = (name: string) => readFileSync(path.join(root, "content/legal", name), "utf8");
const privacy = legal("privacy.md");
const cookies = legal("cookies.md");
const terms = legal("terms.md");

/** collection -> text that privacy.md must contain, or null with a reason when it holds no personal data. */
const COLLECTIONS: Record<string, { privacy: string } | { exempt: string }> = {
  users: { privacy: "Account: name, email" },
  sessions: { privacy: "user agent" },
  progress: { privacy: "Learning progress" },
  xp_events: { privacy: "XP history" },
  achievements: { privacy: "achievements" },
  reviews: { privacy: "review schedule" },
  review_events: { privacy: "log of each review answer" },
  password_resets: { privacy: "Password reset emails" },
  email_verifications: { privacy: "Email verification tokens" },
  email_queue: { privacy: "Queued account emails" },
  auth_events: { privacy: "auth_events" },
  login_failures: { privacy: "Sign-in failure counters" },
  pending_2fa: { privacy: "Pending two-factor sign-ins" },
  rate_limits: { privacy: "Rate limiting counters" },
  events: { privacy: "Analytics (only if you say yes)" },
  tutor_threads: { privacy: "AI tutor conversations" },
  tutor_spend: { privacy: "AI tutor daily spend totals" },
  tutor_usage: { privacy: "tutor_usage" },
  tutor_cache: { privacy: "Saved replies are stored under a one-way fingerprint" },
  admin_audit: { privacy: "Administrator action log" },
  locks: { exempt: "email-drainer lease: a name and an expiry time, no personal data" },
};

const walk = (dir: string): string[] =>
  readdirSync(dir).flatMap((f) => {
    const p = path.join(dir, f);
    return statSync(p).isDirectory() ? walk(p) : /\.(ts|tsx|mts)$/.test(f) ? [p] : [];
  });
const files = walk(path.join(root, "src"));
const source = files.map((f) => readFileSync(f, "utf8"));
const problems: string[] = [];

// 1. Collections.
const used = new Set<string>();
for (const text of source) for (const m of text.matchAll(/\.collection(?:<[^>]*>)?\(\s*"([a-z_0-9]+)"/g)) used.add(m[1]);
for (const name of [...used].sort()) {
  const rule = COLLECTIONS[name];
  if (!rule) problems.push(`collection "${name}" is used in src/ but is not mapped in scripts/checks/legal-drift.mts (add it to privacy.md and the map)`);
  else if ("privacy" in rule && !privacy.includes(rule.privacy)) problems.push(`collection "${name}": privacy.md no longer contains "${rule.privacy}"`);
}
for (const name of Object.keys(COLLECTIONS)) if (!used.has(name)) problems.push(`mapped collection "${name}" is not used in src/ any more (remove it from the map and the privacy table)`);

// 2. Browser storage keys: every literal or template-literal key passed to localStorage/sessionStorage or named bm:* / bm_* must be in cookies.md.
const keys = new Set<string>();
for (const text of source) {
  for (const m of text.matchAll(/["'`](bm:[a-z][a-z0-9:_-]*)(?:\$\{|["'`])/g)) keys.add(m[1]);
}
// In-page events (CustomEvent names), not storage.
const EVENTS = new Set(["bm:consent-changed", "bm:open-consent", "bm:theme-changed", "bm:lesson-jump"]);
for (const key of [...keys].sort()) {
  if (EVENTS.has(key)) continue;
  if (!cookies.includes(key)) problems.push(`browser storage key "${key}" is used in src/ but cookies.md does not mention it`);
}
// Keys written through a variable are invisible to the scan above, so list new storage calls that do not use a known pattern.
for (const [i, text] of source.entries()) {
  for (const m of text.matchAll(/(?:localStorage|sessionStorage)\.setItem\(\s*([^,)]+)/g)) {
    const arg = m[1].trim();
    if (/^(key|KEY|storageKey|preferenceKey|splitKey)$/.test(arg)) continue; // constants are checked through the bm: scan above
    problems.push(`${path.relative(root, files[i])}: storage key expression "${arg}" is not recognised; check cookies.md and teach this script`);
  }
}

// 3. Cookies set by the app.
const cookieNames = new Set<string>();
for (const text of source) for (const m of text.matchAll(/["'`](bm_[a-z0-9_]+)["'`]/g)) cookieNames.add(m[1]);
const NOT_COOKIES = /^(bm_base|bm_init|bm_test|bm_nonce|bm_hook|bm_take|bm_main|bm_tests?)\b/;
// Names built as `${prefix}bm_session` have no quote right before bm_.
for (const text of source) for (const m of text.matchAll(/\$\{[a-zA-Z]+\}(bm_[a-z0-9_]+)`/g)) cookieNames.add(m[1]);
for (const name of [...cookieNames].sort()) {
  if (NOT_COOKIES.test(name) || name.endsWith("_")) continue;
  if (!cookies.includes(name)) problems.push(`"${name}" looks like a cookie but cookies.md does not list it`);
}

// 4. Languages: the legal pages must not name a subset of the tracks.
for (const [file, text] of [["privacy.md", privacy], ["terms.md", terms]] as const) {
  if (/\bC and Rust\b/.test(text)) problems.push(`${file} still says "C and Rust"; list every language`);
  for (const lang of ["Go", "assembly"]) if (!text.includes(lang)) problems.push(`${file} does not mention ${lang}`);
}

// 5. Template placeholders and duplicated paragraphs.
for (const [file, text] of [["privacy.md", privacy], ["cookies.md", cookies], ["terms.md", terms]] as const) {
  if (!text.includes("{{lastUpdated}}")) problems.push(`${file} lost its {{lastUpdated}} placeholder`);
  const paras = text.split(/\n{2,}/).map((p) => p.trim()).filter((p) => p.length > 80);
  const dup = paras.find((p, i) => paras.indexOf(p) !== i);
  if (dup) problems.push(`${file} repeats a paragraph: "${dup.slice(0, 60)}..."`);
  // Two sentences that restate the same setting under the same key are the usual drift: flag a storage key described twice in separate paragraphs.
  if (file === "cookies.md") for (const key of ["bm:editor:font-size", "bm:editor:tab-width", "bm:workspace:tour-done"]) {
    const n = paras.filter((p) => p.includes(key)).length;
    if (n > 1) problems.push(`cookies.md describes ${key} in ${n} paragraphs`);
  }
}

if (problems.length) {
  console.error(`Legal drift (${problems.length}):\n - ${problems.join("\n - ")}\nReminder: bump SITE.legal.lastUpdated in src/lib/site.ts when content/legal/ changes.`);
  process.exit(1);
}
console.log(`Legal pages cover ${used.size} collections, ${keys.size - EVENTS.size} storage keys and ${cookieNames.size} cookie-like names.`);
