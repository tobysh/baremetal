// Run against local Mongo: node --conditions=react-server --env-file=.env.local --import tsx scripts/security/xp-atomic.mts
import assert from "node:assert/strict";
import { ObjectId } from "mongodb";
import { getDb } from "../../src/lib/db";
import { reconcilePendingXp, recordAttempt, saveCode } from "../../src/lib/progress";

const db = await getDb();
const userId = new ObjectId(), id = userId.toHexString();
const users = db.collection("users");
const attempt = (exerciseId: string, difficulty: "easy" | "impossible" = "easy") => recordAttempt({ userId: id, exerciseId, trackSlug: "fixture", code: "", passed: true, difficulty, challenge: false });
try {
  await users.insertOne({ _id: userId, xp: 0 });
  const results = await Promise.all(Array.from({ length: 12 }, (_, i) => attempt(`fixture/${i}`)));
  assert.equal(results.filter((r) => r.xpAwarded === 5).length, 6, "parallel passes must respect the pace cap");
  assert.equal(results.filter((r) => r.paced).length, 6);
  assert.equal((await users.findOne({ _id: userId }))?.xp, 30);
  assert.equal(await db.collection("xp_events").countDocuments({ userId }), 6);

  await db.collection("progress").deleteMany({ userId });
  await db.collection("xp_events").deleteMany({ userId });
  await users.updateOne({ _id: userId }, { $set: { xp: 390, xpQuota: [{ at: new Date(Date.now() - 3600_000), xp: 390 }] } });
  const daily = await attempt("fixture/day", "impossible");
  assert.equal(daily.xpAwarded, 0, "daily cap should withhold the whole award");
  assert.equal((await db.collection("progress").findOne({ userId, exerciseId: "fixture/day" }))?.xpFlag, "daily-cap");

  await db.collection("progress").deleteMany({ userId });
  await db.collection("xp_events").deleteMany({ userId });
  await users.updateOne({ _id: userId }, { $set: { xp: 390, xpQuota: [{ at: new Date(Date.now() - 3600_000), xp: 390 }] } });
  const dailyRace = await Promise.all(Array.from({ length: 12 }, (_, i) => attempt(`fixture/daily-${i}`)));
  assert.equal(dailyRace.reduce((n, r) => n + r.xpAwarded, 0), 10, "parallel passes must respect the daily cap");
  assert.equal((await users.findOne({ _id: userId }))?.xp, 400);

  await db.collection("progress").deleteMany({ userId });
  await db.collection("xp_events").deleteMany({ userId });
  await users.updateOne({ _id: userId }, { $set: { xp: 0, xpQuota: [] } });
  const same = await Promise.all(Array.from({ length: 12 }, () => attempt("fixture/same")));
  assert.equal(same.filter((r) => r.firstPass).length, 1, "concurrent duplicate submissions must award once");
  assert.equal(await db.collection("xp_events").countDocuments({ userId }), 1);

  await db.collection("progress").deleteMany({ userId });
  await users.updateOne({ _id: userId }, { $set: { xp: 0, xpQuota: [] } });
  const first = await attempt("fixture/first");
  assert.equal(first.xpAwarded, 5, "a first submission with unknown start must not be automatically flagged");
  await saveCode(id, "fixture/fast", "fixture", "int x;");
  const fast = await attempt("fixture/fast");
  assert.equal(fast.xpAwarded, 0);
  assert.equal((await db.collection("progress").findOne({ userId, exerciseId: "fixture/fast" }))?.xpFlag, "too-fast");

  // A crash after the claim leaves xpPending; the reconciler settles each shape of it.
  await db.collection("progress").deleteMany({ userId });
  await db.collection("xp_events").deleteMany({ userId });
  const old = new Date(Date.now() - 3_600_000), ancient = new Date(Date.now() - 3 * 86_400_000);
  const pending = (exerciseId: string, passedAt: Date) => db.collection("progress").insertOne({ userId, exerciseId, trackSlug: "fixture", status: "passed", passedAt, xpPending: true, xpAwarded: 0, code: "", attempts: 1 });
  await users.updateOne({ _id: userId }, { $set: { xp: 5, xpQuota: [{ ref: "fixture/decided", at: old, xp: 5 }] } });
  await pending("fixture/decided", old);
  await pending("fixture/undecided", old);
  await pending("fixture/ancient", ancient);
  const rec = await reconcilePendingXp(60_000);
  assert.deepEqual(rec, { finished: 1, reverted: 1, unresolved: 1 });
  const decided = await db.collection("progress").findOne({ userId, exerciseId: "fixture/decided" });
  assert.equal(decided?.xpPending, undefined);
  assert.equal(decided?.xpAwarded, 5);
  assert.equal(await db.collection("xp_events").countDocuments({ userId, ref: "fixture/decided", xp: 5 }), 1);
  assert.equal((await users.findOne({ _id: userId }))?.xp, 5, "reconciling must not award twice");
  const undecided = await db.collection("progress").findOne({ userId, exerciseId: "fixture/undecided" });
  assert.equal(undecided?.status, "started");
  assert.equal(undecided?.xpPending, undefined);
  assert.equal((await db.collection("progress").findOne({ userId, exerciseId: "fixture/ancient" }))?.xpPending, true);
  assert.deepEqual(await reconcilePendingXp(60_000), { finished: 0, reverted: 0, unresolved: 1 }, "second run is idempotent");
  console.log("XP atomicity regressions passed.");
} finally {
  await Promise.all([users.deleteOne({ _id: userId }), db.collection("progress").deleteMany({ userId }), db.collection("xp_events").deleteMany({ userId }), db.collection("reviews").deleteMany({ userId })]);
}
process.exit(0);
