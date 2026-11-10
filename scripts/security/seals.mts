// Isolated Mongo regression: node --conditions=react-server --import tsx scripts/security/seals.mts
import assert from "node:assert/strict";
import { ObjectId, type MongoClient } from "mongodb";
import { randomBytes } from "node:crypto";
import { achievementById } from "../../src/lib/achievements";

const scratchDbName = `bm_seals_${randomBytes(8).toString("hex")}`;
process.env.MONGODB_DB = scratchDbName;
const { getDb } = await import("../../src/lib/db");
const { recordAttempt, recordHint } = await import("../../src/lib/progress");
const { evaluateAchievements } = await import("../../src/lib/rewards");
const db = await getDb();
if (db.databaseName !== scratchDbName || !db.databaseName.startsWith("bm_seals_")) throw new Error("Refusing to run seal regressions outside their unique scratch database");
const progress = db.collection("progress");
const exerciseId = "fixture/seal";
async function learner() {
  const userId = new ObjectId();
  await db.collection("users").insertOne({ _id: userId, email: `seal-${userId}@example.test`, xp: 0 });
  return userId;
}
const pass = (userId: ObjectId, difficulty: "easy" | "medium" = "easy", extra = {}) => recordAttempt({ userId: userId.toHexString(), exerciseId, trackSlug: "fixture", code: "saved code", passed: true, difficulty, ...extra });
let failed = false;
try {
  for (const difficulty of ["easy", "medium"] as const) {
    const userId = await learner();
    const result = await pass(userId, difficulty);
    assert.equal(result.unaided, true, `${difficulty} first-pass completion is unaided without assistance`);
    const stored = await progress.findOne({ userId, exerciseId });
    assert.equal(stored?.attemptsAtPass, 1);
    assert.equal(stored?.unaided, true);
    assert.equal(result.xpAwarded, difficulty === "easy" ? 5 : 10, "cosmetic eligibility does not change XP");
  }

  const retry = await learner();
  await recordAttempt({ userId: retry.toHexString(), exerciseId, trackSlug: "fixture", code: "failed code", passed: false, difficulty: "easy" });
  await progress.updateOne({ userId: retry, exerciseId }, { $set: { startedAt: new Date(Date.now() - 60_000) } });
  assert.equal((await pass(retry)).unaided, true, "failed attempts do not change assistance status");
  assert.equal((await progress.findOne({ userId: retry, exerciseId }))?.attemptsAtPass, 2);

  const hinted = await learner();
  await recordHint(hinted.toHexString(), exerciseId, "fixture", 0);
  await progress.updateOne({ userId: hinted, exerciseId }, { $set: { startedAt: new Date(Date.now() - 60_000) } });
  const hintedPass = await pass(hinted);
  assert.equal(hintedPass.unaided, false);
  assert.equal(hintedPass.xpAwarded, 5, "appropriate hint use preserves full XP");
  assert.equal((await progress.findOne({ userId: hinted, exerciseId }))?.hintsUsedAtPass, 1);

  const tutor = await learner();
  await progress.insertOne({ userId: tutor, exerciseId, trackSlug: "fixture", status: "started", attempts: 0, tutorUsed: true, startedAt: new Date(Date.now() - 60_000) });
  await db.collection("tutor_threads").insertOne({ userId: tutor, lessonId: exerciseId, messages: [{ role: "user", content: "hint please" }] });
  await db.collection("tutor_threads").deleteMany({ userId: tutor });
  assert.equal((await pass(tutor)).unaided, false, "clearing tutor history cannot clear sticky assistance");

  const legacyTutor = await learner();
  await db.collection("tutor_threads").insertOne({ userId: legacyTutor, lessonId: exerciseId, messages: [{ role: "user", content: "a question" }] });
  assert.equal((await pass(legacyTutor)).unaided, false, "legacy tutor history still establishes assistance");

  const earned = await learner();
  await pass(earned);
  const snapshot = await progress.findOne({ userId: earned, exerciseId });
  const earnedAt = new Date("2026-01-02T00:00:00Z");
  await db.collection("achievements").insertOne({ userId: earned, id: "unaided-5", unlockedAt: earnedAt });
  await recordHint(earned.toHexString(), exerciseId, "fixture", 0);
  await progress.updateOne({ userId: earned, exerciseId }, { $set: { tutorUsed: true } });
  assert.equal((await pass(earned)).firstPass, false);
  const after = await progress.findOne({ userId: earned, exerciseId });
  for (const field of ["unaided", "attemptsAtPass", "hintsUsedAtPass", "xpAwarded"] as const) assert.deepEqual(after?.[field], snapshot?.[field], `repeat runs preserve ${field}`);
  await evaluateAchievements(earned.toHexString());
  assert.equal((await db.collection("achievements").findOne({ userId: earned, id: "unaided-5" }))?.unlockedAt.getTime(), earnedAt.getTime(), "previously earned achievements and dates are preserved");

  const previousAssisted = await learner();
  await progress.insertOne({ userId: previousAssisted, exerciseId, status: "passed", attempts: 1, attemptsAtPass: 1, hintsUsedAtPass: 0, unaided: false, xpAwarded: 5, passedAt: earnedAt });
  await pass(previousAssisted);
  assert.equal((await progress.findOne({ userId: previousAssisted, exerciseId }))?.unaided, false, "new rule does not rewrite stored historical seals");

  for (const variant of [{ challenge: true }, { practice: true }]) {
    const userId = await learner();
    assert.equal((await pass(userId, "medium", variant)).unaided, false, "challenge/practice passes do not earn lesson seals");
  }
  for (const id of ["review-1", "review-10", "revisit-track"]) assert.match(achievementById.get(id)!.description, /coding review/i);
  console.log("Unaided first-pass/retry assistance rules, sticky tutor history, earned-seal preservation and coding-review descriptions passed.");
} catch (error) {
  failed = true;
  console.error(error);
} finally {
  await db.dropDatabase();
  await (globalThis as unknown as { __mongo?: { client: MongoClient } }).__mongo?.client.close();
}
if (failed) process.exitCode = 1;
