import test from "node:test";
import assert from "node:assert/strict";
import { ObjectId } from "mongodb";
import { achievementById } from "../../src/lib/achievements";
import { getTracks } from "../../src/lib/curriculum";
import { buildContext, sealsFor } from "../../src/lib/rewards-context";
import type { ProgressDoc, ReviewDoc } from "../../src/lib/models";

const tracks = getTracks();
const medium = tracks.flatMap((track) => track.lessons).find((lesson) => lesson.difficulty === "medium")!;
const now = new Date("2026-11-10T12:00:00Z");
const userId = new ObjectId();
const passed = (attemptsAtPass: number): ProgressDoc => ({ _id: new ObjectId(), userId, exerciseId: medium.exercise.id, trackSlug: medium.exercise.id.split("/")[0], status: "passed", code: "", attempts: attemptsAtPass, attemptsAtPass, unaided: true, passedAt: now, updatedAt: now });

test("Unaided assistance status and first-attempt recognition remain separate", () => {
  const first = buildContext(tracks, [passed(1)], [], {}, now);
  const retry = buildContext(tracks, [passed(2)], [], {}, now);
  assert.equal(first.passed.get(medium.exercise.id)?.unaided, true);
  assert.equal(retry.passed.get(medium.exercise.id)?.unaided, true);
  assert.equal(achievementById.get("first-try")!.check(first), true);
  assert.equal(achievementById.get("first-try")!.check(retry), false);
});

test("stored earned seals are not recalculated from later assistance or attempts", () => {
  const progress = { ...passed(1), hintsUsed: 3, tutorUsed: true, attempts: 12 };
  const seals = sealsFor(new Map([[progress.exerciseId, progress]]), progress.exerciseId);
  assert.equal(seals.unaided, true);
  assert.equal(buildContext(tracks, [progress], [], {}, now).passed.get(progress.exerciseId)?.unaided, true);
});

test("coding-review recognition keeps existing thresholds and accurate descriptions", () => {
  for (const count of [0, 1, 9, 10]) {
    const review: ReviewDoc = { _id: new ObjectId(), userId, exerciseId: medium.exercise.id, dueAt: now, lastAt: now, interval: 7, count };
    const context = buildContext(tracks, [], [review], {}, now);
    assert.equal(achievementById.get("review-1")!.check(context), count >= 1);
    assert.equal(achievementById.get("review-10")!.check(context), count >= 10);
  }
  for (const id of ["review-1", "review-10", "revisit-track"]) assert.match(achievementById.get(id)!.description, /coding review/i);
});
