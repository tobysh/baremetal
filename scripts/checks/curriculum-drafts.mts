// Explicit drafts may be incomplete; published content must remain strict.
import assert from "node:assert/strict";
import { execFileSync } from "node:child_process";
import { cpSync, mkdirSync, mkdtempSync, readFileSync, rmSync, writeFileSync } from "node:fs";
import { createRequire } from "node:module";
import { tmpdir } from "node:os";
import path from "node:path";
import { pathToFileURL } from "node:url";

const root = path.resolve(import.meta.dirname, "../..");
const scratch = mkdtempSync(path.join(tmpdir(), "bm-curriculum-drafts-"));
const loader = createRequire(import.meta.url).resolve("tsx");
const moduleUrl = pathToFileURL(path.join(root, "src/lib/curriculum/index.ts")).href;
const inspect = (only?: string) => JSON.parse(execFileSync(process.execPath, [
  "--import", loader, "--input-type=module", "--eval",
  `import { getTracks } from ${JSON.stringify(moduleUrl)};
   console.log(JSON.stringify(getTracks(${JSON.stringify(only)}).map(t => ({slug:t.slug, lessons:t.lessons.map(l => l.slug)}))));`,
], { cwd: scratch, env: { ...process.env, NODE_ENV: "test" }, encoding: "utf8", stdio: "pipe" }));
try {
  const published = path.join(scratch, "content/published");
  mkdirSync(published, { recursive: true });
  writeFileSync(path.join(published, "track.json"), JSON.stringify({ title: "Published", language: "c", path: "start", order: 1 }));
  cpSync(path.join(root, "content/first-steps/01-hello-world"), path.join(published, "01-hello-world"), { recursive: true });
  const draftLesson = path.join(published, "02-incomplete");
  mkdirSync(draftLesson);
  const draftText = "---\ndraft: true\ntitle: Not ready\n---\n";
  writeFileSync(path.join(draftLesson, "lesson.md"), draftText);
  const draftTrack = path.join(scratch, "content/draft-course");
  mkdirSync(path.join(draftTrack, "01-missing-lesson"), { recursive: true });
  writeFileSync(path.join(draftTrack, "track.json"), JSON.stringify({ draft: true, language: "c" }));
  assert.deepEqual(inspect(), [{ slug: "published", lessons: ["hello-world"] }]);
  assert.deepEqual(inspect("draft-course"), [], "direct course lookup must also hide drafts");
  writeFileSync(path.join(draftLesson, "lesson.md"), draftText.replace("draft: true\n", ""));
  assert.throws(() => inspect(), /task\.md/, "an incomplete published lesson must fail, not silently disappear");
  writeFileSync(path.join(draftLesson, "lesson.md"), draftText);
  writeFileSync(path.join(draftTrack, "track.json"), JSON.stringify({ language: "c" }));
  assert.throws(() => inspect(), /lesson\.md/, "an incomplete published course must fail");
  // A string is not the boolean flag documented for track.json.
  writeFileSync(path.join(draftTrack, "track.json"), JSON.stringify({ draft: "true", language: "c" }));
  assert.throws(() => inspect(), /lesson\.md/, "malformed track flags must not hide published content");
  assert.match(readFileSync(path.join(draftLesson, "lesson.md"), "utf8"), /draft: true/);
  console.log("Curriculum draft isolation and strict published-content checks passed.");
} finally {
  rmSync(scratch, { recursive: true, force: true });
}
