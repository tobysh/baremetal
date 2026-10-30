import assert from "node:assert/strict";
import { getTracks } from "../../src/lib/curriculum";
import { recommendLearning, validateLearningPaths, type LearningProgress } from "../../src/lib/learning";
import { buildLearningCohorts, type LearnerCourse } from "../../src/lib/learning-cohorts";
import { explainFailure, type FailureInput } from "../../src/lib/diagnostics";

const tracks = getTracks();
validateLearningPaths(tracks);
const zero = tracks.find((track) => track.slug === "first-steps")!;
const c = tracks.find((track) => track.slug === "c-data")!;
const go = tracks.find((track) => track.slug === "go-foundations")!;
const p = new Map<string, LearningProgress>();
const record = (id: string, status: string, at = 1) => p.set(id, { exerciseId: id, status, updatedAt: new Date(at) });
assert.equal(recommendLearning(tracks, p)?.track.slug, zero.slug);
record(go.lessons[0].exercise.id, "started", 1);
assert.equal(recommendLearning(tracks, p)?.track.slug, go.slug, "skipping prerequisites remains allowed");
record(go.lessons[0].exercise.id, "passed", 2);
assert.equal(recommendLearning(tracks, p)?.lesson.slug, go.lessons[1].slug, "a pass continues the selected course, not catalogue order");
record(zero.lessons[0].exercise.id + "#challenge", "passed", 99);
assert.equal(recommendLearning(tracks, p)?.track.slug, go.slug, "challenges cannot hijack the resume action");
record(go.lessons[4].exercise.id, "started", 3);
assert.equal(recommendLearning(tracks, p)?.lesson.slug, go.lessons[4].slug, "resume the actual unfinished lesson");
p.clear();
for (const lesson of zero.lessons) record(lesson.exercise.id, "passed");
assert.equal(recommendLearning(tracks, p)?.track.slug, c.slug, "beginner course transitions into C");
assert.equal(recommendLearning(tracks, new Map(), { track: go })?.track.slug, go.slug);
const completeGo = new Map(go.lessons.map((lesson) => [lesson.exercise.id, { exerciseId: lesson.exercise.id, status: "passed", updatedAt: new Date(1) }]));
assert.equal(recommendLearning(tracks, completeGo, { track: go })?.track.slug, "go-practice");
const lastGo = tracks.filter((track) => track.path === "go").at(-1)!;
const allPassed = new Map(tracks.flatMap((track) => track.lessons.map((lesson) => [lesson.exercise.id, { exerciseId: lesson.exercise.id, status: "passed", updatedAt: new Date(1) }] as const)));
assert.equal(recommendLearning(tracks, allPassed, { track: lastGo }), null);
assert.throws(() => validateLearningPaths([{ ...zero, prerequisite: "missing" }]), /missing prerequisite/);
assert.throws(() => validateLearningPaths([{ ...zero, prerequisite: c.slug }, c]), /cycle/);
const beforeLast = new Map(zero.lessons.slice(0, -1).map((lesson) => [lesson.exercise.id, { exerciseId: lesson.exercise.id, status: "passed", updatedAt: new Date(1) }]));
assert.equal(recommendLearning(tracks, beforeLast, { track: zero, afterPass: zero.lessons.at(-1)!.exercise.id })?.track.slug, c.slug);
assert(!beforeLast.has(zero.lessons.at(-1)!.exercise.id), "hypothetical completion doesn't mutate progress");

const track = { ...zero, lessons: zero.lessons.slice(0, 2) };
const groups: LearnerCourse[] = Array.from({ length: 5 }, (_, i) => ({ trackSlug: track.slug, lessons: [
  { exerciseId: track.lessons[0].exercise.id, status: "passed", startedAt: new Date("2026-10-05T23:00:00Z"), passedAt: new Date(`2026-10-06T0${i}:00:00Z`) },
  { exerciseId: track.lessons[1].exercise.id, status: i < 3 ? "passed" : "started", startedAt: new Date("2026-10-13T00:00:00Z"), ...(i < 3 ? { passedAt: new Date("2026-10-13T01:00:00Z") } : {}) },
] }));
groups.push({ trackSlug: track.slug, lessons: [{ exerciseId: track.lessons[0].exercise.id, status: "passed" }] });
groups.push({ trackSlug: track.slug, lessons: [{ exerciseId: track.lessons[0].exercise.id, status: "started", startedAt: new Date("2026-10-12T00:00:00Z") }] });
const report = buildLearningCohorts([track], groups, new Date("2026-10-30T00:00:00Z"));
assert.equal(report.rows.length, 1);
assert.equal(report.hidden, 1);
assert.equal(report.excluded, 1);
assert.deepEqual(report.rows[0], { trackSlug: track.slug, title: track.title, week: "2026-10-05", learners: 5, firstLessonPassed: 5, completed: 3, medianFirstPassHours: 3, laterActivity: 5, mature: true });
assert.equal(buildLearningCohorts([track], groups.slice(0, 5), new Date("2026-10-15T00:00:00Z")).rows[0].mature, false);
const sunday = groups.slice(0, 5).map((group) => ({ ...group, lessons: group.lessons.map((lesson) => ({ ...lesson, startedAt: new Date("2026-10-11T23:59:00Z") })) }));
assert.equal(buildLearningCohorts([track], sunday).rows[0].week, "2026-10-05");
assert.equal(buildLearningCohorts([track], groups.slice(0, 4)).rows.length, 0);

const base: FailureInput = { language: "c", mode: "harness", code: "changed", starter: "starter", compileFailed: false, compileOutput: "", error: null, results: [] };
for (const input of [
  { ...base, compileFailed: true, compileOutput: "expected ';'" },
  { ...base, error: "Timed out" },
  { ...base, runtimeFailed: true },
  { ...base, results: [{ name: "ordinary", passed: false }] },
]) assert(explainFailure(input).every((finding) => finding.nextStep.length > 20));
assert(explainFailure({ ...base, runtimeFailed: true }).length > 0, "unknown runtime failures still get a next step");
assert.deepEqual(explainFailure(base), []);
console.log("Learning recommendations, prerequisites, cohorts and diagnostic checks passed.");
