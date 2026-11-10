// Content quality bar. Reports lessons that fall below the minimum for teaching depth, tests and recall.
// Report-only by default (exit 0). `--strict` exits 1 on any finding; `--track <slug>` limits the scan.
// Review lessons are consolidation exercises, so they are exempt from the worked-example rule.
import { existsSync, readFileSync, readdirSync } from "node:fs";
import path from "node:path";

const CONTENT = path.join(process.cwd(), "content");
const strict = process.argv.includes("--strict");
const only = process.argv.includes("--track") ? process.argv[process.argv.indexOf("--track") + 1] : null;

// Generic hint phrases that were pasted across lessons instead of helping with the task at hand.
const BOILERPLATE_HINTS = [
  /reuse the supplied heap traversal/i,
  /test exhaustion and reuse, not only/i,
  /run the starter to inspect the public example/i,
];
const MIN_WORDS: Record<string, number> = { easy: 120, medium: 180, hard: 220, impossible: 220 };
const MAX_STARTER_LINE = 160;
const MIN_HINTS = 2;

type Finding = { lesson: string; rule: string; detail: string };
const findings: Finding[] = [];
const read = (p: string) => readFileSync(p, "utf8");
const words = (s: string) => s.split(/\s+/).filter(Boolean).length;

function testCount(dir: string, files: string[]): number {
  const t = files.find((f) => /^(tests\.|driver\.c)/.test(f));
  if (t) return (read(path.join(dir, t)).match(/CHECK\(|#\[test\]|func Test|t\.Run\(/g) ?? []).length;
  if (files.includes("cases.json")) return (JSON.parse(read(path.join(dir, "cases.json"))) as unknown[]).length;
  return 0;
}

let lessons = 0;
for (const slug of readdirSync(CONTENT).sort()) {
  const trackDir = path.join(CONTENT, slug);
  if (only && slug !== only) continue;
  if (!existsSync(path.join(trackDir, "track.json"))) continue;
  for (const name of readdirSync(trackDir).filter((d) => /^\d+-/.test(d)).sort()) {
    const dir = path.join(trackDir, name);
    const lesson = path.join(dir, "lesson.md");
    if (!existsSync(lesson)) continue;
    const src = read(lesson);
    const head = src.match(/^---\n([\s\S]*?)\n---\n/)?.[1] ?? "";
    if (/^draft:\s*true/m.test(head)) continue;
    lessons++;
    const id = `${slug}/${name}`;
    const files = readdirSync(dir);
    const body = src.replace(/^---\n[\s\S]*?\n---\n/, "");
    const difficulty = head.match(/^difficulty:\s*(\S+)/m)?.[1] ?? "medium";
    const review = /^kind:\s*review/m.test(head);

    const w = words(body);
    if (w < (MIN_WORDS[difficulty] ?? 180) && !review) findings.push({ lesson: id, rule: "length", detail: `${w} words, want at least ${MIN_WORDS[difficulty] ?? 180} for ${difficulty}` });
    if (!review && !/```/.test(body)) findings.push({ lesson: id, rule: "no-example", detail: "no code block or diagram in the lesson body" });

    const hints = read(path.join(dir, "hints.md")).split(/\n===\n/).map((h) => h.trim()).filter(Boolean);
    if (hints.length < MIN_HINTS) findings.push({ lesson: id, rule: "hints", detail: `${hints.length} hint(s), want at least ${MIN_HINTS}` });
    if (hints.some((h) => BOILERPLATE_HINTS.some((re) => re.test(h)))) findings.push({ lesson: id, rule: "boilerplate-hint", detail: "contains a generic hint pasted across lessons" });

    if (testCount(dir, files) < 3) findings.push({ lesson: id, rule: "tests", detail: `${testCount(dir, files)} test case(s), want at least 3` });

    for (const f of files.filter((x) => /^(starter|solution)\./.test(x))) {
      const long = read(path.join(dir, f)).split("\n").filter((l) => l.length > MAX_STARTER_LINE).length;
      if (long) findings.push({ lesson: id, rule: "dense-code", detail: `${f} has ${long} line(s) over ${MAX_STARTER_LINE} characters` });
    }

    if (!files.includes("practice.json") && !files.includes("quiz.json")) findings.push({ lesson: id, rule: "no-recall", detail: "no practice.json or quiz.json, so reviews fall back to re-solving" });
  }
}

const byRule = new Map<string, Finding[]>();
for (const f of findings) byRule.set(f.rule, [...(byRule.get(f.rule) ?? []), f]);
console.log(`Content bar: ${lessons} lessons scanned, ${findings.length} finding(s) across ${new Set(findings.map((f) => f.lesson)).size} lessons.\n`);
for (const [rule, list] of [...byRule].sort((a, b) => b[1].length - a[1].length)) {
  console.log(`${rule}: ${list.length}`);
  const perTrack = new Map<string, number>();
  for (const f of list) perTrack.set(f.lesson.split("/")[0], (perTrack.get(f.lesson.split("/")[0]) ?? 0) + 1);
  console.log(`  ${[...perTrack].sort((a, b) => b[1] - a[1]).map(([t, n]) => `${t} ${n}`).join(", ")}`);
  if (only) for (const f of list) console.log(`    ${f.lesson}: ${f.detail}`);
}
if (strict && findings.length) process.exit(1);
