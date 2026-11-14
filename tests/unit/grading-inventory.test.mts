import test from "node:test";
import assert from "node:assert/strict";
import { getTracks } from "../../src/lib/curriculum";
import { gradingInventory } from "../../src/lib/grading-inventory";

test("inventory covers every published programming assessment including challenges", () => {
  const tracks = getTracks();
  const rows = gradingInventory(tracks);
  const exercises = tracks.flatMap((track) => track.lessons.flatMap((lesson) => [lesson.exercise, ...(lesson.challenge ? [lesson.challenge.exercise] : [])]));
  assert.equal(rows.length, exercises.length);
  assert(rows.some((row) => row.kind === "challenge"));
  assert(rows.some((row) => row.trust === "driver"));
  assert(rows.some((row) => row.trust === "stdout"));
  assert(rows.some((row) => row.sameProcessVerdict));
  for (const exercise of exercises) {
    const row = rows.find((candidate) => candidate.id === exercise.id)!;
    assert.equal(row.trust, exercise.driver ? "driver" : exercise.mode);
    assert.equal(row.sameProcessVerdict, row.trust === "harness");
    assert.deepEqual(Object.keys(row).sort(), ["id", "kind", "language", "sameProcessVerdict", "track", "trust"]);
  }
});

test("inventory rejects duplicate ids instead of silently hiding assessments", () => {
  const track = getTracks()[0];
  assert.throws(() => gradingInventory([track, track]), /Duplicate assessment id/);
});
