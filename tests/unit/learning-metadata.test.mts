import test from "node:test";
import assert from "node:assert/strict";
import { execFileSync } from "node:child_process";
import { cpSync, mkdirSync, mkdtempSync, rmSync, writeFileSync } from "node:fs";
import { createRequire } from "node:module";
import { tmpdir } from "node:os";
import path from "node:path";
import { pathToFileURL } from "node:url";
import { parseLearningMetadata } from "../../src/lib/curriculum/learning-metadata";
import { getTracks } from "../../src/lib/curriculum";
import { toPublicExercise } from "../../src/lib/curriculum/types";

test("optional metadata accepts independent fields and trims authored text", () => {
  assert.deepEqual(parseLearningMetadata({ objectives: [" Explain pointer values. "] }), { objectives: ["Explain pointer values."] });
  assert.deepEqual(parseLearningMetadata({ prerequisites: ["Integer variables"], concepts: ["c.pointer-addresses"] }), { prerequisites: ["Integer variables"], concepts: ["c.pointer-addresses"] });
});

test("malformed metadata fails visibly rather than silently losing author intent", () => {
  for (const value of [null, [], "text", {}, { objective: ["typo"] }, { objectives: "text" }, { objectives: [] }, { objectives: [" "] }, { prerequisites: [3] }, { objectives: ["Trace x", " Trace x "] }, { concepts: ["c.pointer-addresses", "c.pointer-addresses"] }]) {
    assert.throws(() => parseLearningMetadata(value, "fixture/learning.json"), /fixture\/learning\.json:/);
  }
  for (const concept of ["pointers", "C.Pointers", "c._pointers", ".pointers", "c.pointers.", "c..pointers", "c.pointer addresses"]) {
    assert.throws(() => parseLearningMetadata({ concepts: [concept] }), /namespaced ID/);
  }
});

test("published examples map existing lessons without altering their public exercise boundary", () => {
  const lessons = getTracks("c-memory")[0].lessons;
  for (const slug of ["pointers-swap", "arrays"]) {
    const lesson = lessons.find((candidate) => candidate.slug === slug)!;
    assert(lesson.learning?.objectives?.length);
    assert(lesson.learning?.prerequisites?.length);
    assert(lesson.learning?.concepts?.length);
    const publicExercise = toPublicExercise(lesson.exercise);
    assert.equal(publicExercise.id, `c-memory/${slug}`);
    assert(Object.keys(publicExercise).every((key) => ["hintCount", "id", "language", "mode", "revision", "starter", "task"].includes(key)));
    for (const key of ["solution", "harness", "driver", "hints", "cases", "learning"]) assert(!(key in publicExercise));
  }
});

test("loader keeps absent sidecars compatible, validates published metadata and ignores drafts", () => {
  const root = path.resolve(import.meta.dirname, "../..");
  const scratch = mkdtempSync(path.join(tmpdir(), "bm-learning-metadata-"));
  const loader = createRequire(import.meta.url).resolve("tsx");
  const moduleUrl = pathToFileURL(path.join(root, "src/lib/curriculum/index.ts")).href;
  const inspect = () => JSON.parse(execFileSync(process.execPath, ["--import", loader, "--input-type=module", "--eval", `import { getTracks } from ${JSON.stringify(moduleUrl)}; console.log(JSON.stringify(getTracks().flatMap(t => t.lessons.map(l => ({id:l.exercise.id, learning:l.learning})))));`], { cwd: scratch, env: { ...process.env, NODE_ENV: "test" }, encoding: "utf8", stdio: "pipe" }));
  try {
    const track = path.join(scratch, "content/fixture");
    const lesson = path.join(track, "01-hello-world");
    mkdirSync(track, { recursive: true });
    writeFileSync(path.join(track, "track.json"), JSON.stringify({ title: "Fixture", language: "c", path: "start", order: 1 }));
    cpSync(path.join(root, "content/first-steps/01-hello-world"), lesson, { recursive: true });
    assert.deepEqual(inspect(), [{ id: "first-steps/hello-world" }]);
    const metadata = { objectives: ["Print a line."], concepts: ["c.standard-output"] };
    writeFileSync(path.join(lesson, "learning.json"), JSON.stringify(metadata));
    assert.deepEqual(inspect(), [{ id: "first-steps/hello-world", learning: metadata }]);
    writeFileSync(path.join(lesson, "learning.json"), "{");
    assert.throws(inspect, /learning\.json is not valid JSON/);
    writeFileSync(path.join(lesson, "learning.json"), '{"concepts":["Typo"]}');
    assert.throws(inspect, /namespaced ID/);
    writeFileSync(path.join(lesson, "lesson.md"), "---\ndraft: true\n---\nUnpublished");
    assert.deepEqual(inspect(), [], "invalid metadata must not publish or invalidate an explicit draft");
    writeFileSync(path.join(track, "track.json"), JSON.stringify({ draft: true, language: "c" }));
    writeFileSync(path.join(lesson, "lesson.md"), "broken unpublished content");
    assert.deepEqual(inspect(), [], "draft courses remain excluded before lesson loading");
  } finally { rmSync(scratch, { recursive: true, force: true }); }
});
