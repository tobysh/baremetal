// Fails if a published lesson id (the key of the `progress` collection) disappears.
// Compares content/**/lesson.md `id:` values against the committed snapshot
// content/.lesson-ids.json, and against the snapshot on origin/main when available.
// New ids are allowed. Regenerate the snapshot with: npm run check:ids -- --update
// (the update only ever adds ids; removing one needs a hand edit and a good reason).
import { readdirSync, readFileSync, writeFileSync, existsSync } from "node:fs";
import { join } from "node:path";
import { execFileSync } from "node:child_process";

const root = join(import.meta.dirname, "..", "content");
const snapPath = join(root, ".lesson-ids.json");

function currentIds(): string[] {
  const ids: string[] = [];
  for (const track of readdirSync(root, { withFileTypes: true })) {
    if (!track.isDirectory()) continue;
    for (const lesson of readdirSync(join(root, track.name), { withFileTypes: true })) {
      const f = join(root, track.name, lesson.name, "lesson.md");
      if (!lesson.isDirectory() || !existsSync(f)) continue;
      const fm = /^---\r?\n([\s\S]*?)\r?\n---/.exec(readFileSync(f, "utf8"));
      const m = fm && /^id:\s*(\S+)\s*$/m.exec(fm[1]);
      if (!m) {
        console.error(`missing id: in ${track.name}/${lesson.name}/lesson.md`);
        process.exit(1);
      }
      ids.push(m[1].replace(/^["']|["']$/g, ""));
    }
  }
  return ids.sort();
}

const now = currentIds();
const dupes = now.filter((id, i) => now.indexOf(id) !== i);
if (dupes.length) {
  console.error("duplicate lesson ids:", [...new Set(dupes)].join(", "));
  process.exit(1);
}

const read = (p: string): string[] => JSON.parse(readFileSync(p, "utf8"));
if (process.argv.includes("--update")) {
  const old = existsSync(snapPath) ? read(snapPath) : [];
  const merged = [...new Set([...old, ...now])].sort();
  writeFileSync(snapPath, JSON.stringify(merged, null, 2) + "\n");
  console.log(`snapshot updated: ${merged.length} ids (${merged.length - old.length} added)`);
  process.exit(0);
}

const baselines: [string, string[]][] = [];
if (existsSync(snapPath)) baselines.push(["content/.lesson-ids.json", read(snapPath)]);
else {
  console.error("content/.lesson-ids.json is missing; run npm run check:ids -- --update");
  process.exit(1);
}
try {
  const out = execFileSync("git", ["show", "origin/main:content/.lesson-ids.json"], {
    encoding: "utf8",
    stdio: ["ignore", "pipe", "ignore"],
  });
  baselines.push(["origin/main snapshot", JSON.parse(out)]);
} catch {
  // no origin/main or no snapshot there yet: the committed file is the only baseline
}

const have = new Set(now);
let bad = false;
for (const [name, ids] of baselines) {
  const gone = ids.filter((id) => !have.has(id));
  if (gone.length) {
    bad = true;
    console.error(`published lesson ids missing vs ${name} (progress would be lost):`);
    for (const id of gone) console.error(`  ${id}`);
  }
}
const known = new Set(baselines.flatMap(([, ids]) => ids));
const fresh = now.filter((id) => !known.has(id));
if (fresh.length) console.log(`note: ${fresh.length} new id(s) not in snapshot; run npm run check:ids -- --update and commit`);
if (bad) process.exit(1);
console.log(`lesson ids ok (${now.length} lessons)`);
