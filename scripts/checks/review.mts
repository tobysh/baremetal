// Review schedule checks against a scratch database.
//   npm run check:review      (needs local MongoDB: npm run db)
import assert from "node:assert/strict";
import { randomBytes } from "node:crypto";
import { ObjectId, type MongoClient } from "mongodb";

process.env.MONGODB_DB = `bm_review_${randomBytes(6).toString("hex")}`;
const { DAY, jitterDays, startOfLocalDay } = await import("../../src/lib/review-math");
const { getDb } = await import("../../src/lib/db");
const { settleReviewCard, skipReview, logRetry } = await import("../../src/lib/review-cards");
const { recordReviewPass } = await import("../../src/lib/progress");

// ---- jitter stays in bounds ----
for (const i of [3, 7, 21, 60, 240]) {
  const span = i >= 7 ? Math.max(1, i * 0.1) : i * 0.1;
  assert.ok(Math.abs(jitterDays(i, () => 0) - (i - span)) < 1e-9, `${i}: low edge`);
  assert.ok(Math.abs(jitterDays(i, () => 0.999999) - (i + span)) < 1e-4, `${i}: high edge`);
  for (let n = 0; n < 500; n++) {
    const j = jitterDays(i);
    assert.ok(j >= i - span - 1e-9 && j <= i + span + 1e-9, `${i}: ${j} out of bounds`);
  }
}
assert.ok(jitterDays(7, () => 0) <= 6.0001 && jitterDays(7, () => 0.9999) >= 7.99, "7 days jitters by a full day");
assert.equal(jitterDays(3, () => 0.5), 3, "midpoint is the nominal interval");

// ---- local day boundaries ----
assert.equal(startOfLocalDay(new Date("2026-11-05T15:30:00Z"), "UTC", 1).toISOString(), "2026-11-06T00:00:00.000Z");
assert.equal(startOfLocalDay(new Date("2026-11-05T15:30:00Z"), "Europe/London", 1).toISOString(), "2026-11-06T00:00:00.000Z");
assert.equal(startOfLocalDay(new Date("2026-07-05T15:30:00Z"), "Europe/London", 1).toISOString(), "2026-07-05T23:00:00.000Z", "BST midnight is 23:00Z");
assert.equal(startOfLocalDay(new Date("2026-07-05T23:30:00Z"), "Europe/London", 0).toISOString(), "2026-07-05T23:00:00.000Z", "23:30Z is already the next BST day");
assert.equal(startOfLocalDay(new Date("2026-11-05T15:30:00Z"), "Not/AZone", 1).toISOString(), "2026-11-06T00:00:00.000Z", "invalid zone falls back to UTC");

// ---- schedule + event log ----
const db = await getDb();
const reviews = db.collection("reviews"), events = db.collection("review_events");
try {
  assert.ok((await events.indexes()).some((i) => i.key.userId === 1 && i.key.at === 1), "index on { userId, at }");
  const userId = new ObjectId();
  // Recovery must not recreate history for a deleted account.
  await db.collection("users").insertOne({ _id: userId, email: "review-fixture@example.test" });
  const uid = userId.toHexString();
  const seed = async (exerciseId: string, interval = 7, extra = {}) => {
    const past = new Date(Date.now() - 2 * DAY);
    await reviews.insertOne({ userId, exerciseId, dueAt: past, interval, lastAt: new Date(past.getTime() - interval * DAY), count: 0, ...extra });
  };

  // A replayed card answer writes one event.
  await seed("t/card");
  const [first, replay] = [await settleReviewCard(uid, "t/card", { correct: true, questionIndex: 1 }), await settleReviewCard(uid, "t/card", { correct: true, questionIndex: 1 })];
  assert.ok(first && !replay, "second identical answer is refused");
  const parallel = await Promise.all([settleReviewCard(uid, "t/card", { correct: false, questionIndex: 0 }), settleReviewCard(uid, "t/card", { correct: false, questionIndex: 0 })]);
  assert.equal(parallel.filter(Boolean).length, 0, "not due any more");
  const cardEvents = await events.find({ userId, exerciseId: "t/card" }).toArray();
  assert.equal(cardEvents.length, 1);
  const ev = cardEvents[0];
  assert.equal(ev.source, "card"); assert.equal(ev.questionIndex, 1); assert.equal(ev.correct, true); assert.equal(ev.rating, "good");
  assert.equal(ev.intervalBefore, 7); assert.equal(ev.intervalAfter, 21);
  assert.ok(Math.abs(ev.daysLate - 2) < 0.01 && Math.abs(ev.elapsedDays - 9) < 0.01, `daysLate ${ev.daysLate}, elapsed ${ev.elapsedDays}`);
  const afterCard = (await reviews.findOne({ userId, exerciseId: "t/card" }))!;
  assert.equal(afterCard.interval, 21);
  const days = (afterCard.dueAt.getTime() - Date.now()) / DAY;
  assert.ok(days >= 21 - 2.1 - 0.01 && days <= 21 + 2.1 + 0.01, `dueAt jitter in bounds (${days})`);
  assert.equal(afterCard.count, 0, "cards never add to count");

  // A concurrent double answer on a due review also writes one event.
  await seed("t/race", 3);
  const race = await Promise.all([settleReviewCard(uid, "t/race", { correct: false, questionIndex: 0 }), settleReviewCard(uid, "t/race", { correct: false, questionIndex: 0 })]);
  assert.equal(race.filter(Boolean).length, 1);
  assert.equal(await events.countDocuments({ userId, exerciseId: "t/race" }), 1);

  // Partly right is Hard; wrong with nothing right is Again.
  await seed("t/hard"); await settleReviewCard(uid, "t/hard", { correct: false, partial: { right: 3, total: 5 }, questionIndex: 0 });
  const hard = (await events.findOne({ userId, exerciseId: "t/hard" }))!;
  assert.equal(hard.rating, "hard"); assert.equal(hard.partial, "3 of 5");
  await seed("t/again"); await settleReviewCard(uid, "t/again", { correct: false, partial: { right: 0, total: 5 }, questionIndex: 0 });
  assert.equal((await events.findOne({ userId, exerciseId: "t/again" }))!.rating, "again");

  // Retry: once per miss, only the offered question, never moves the schedule.
  const dueBefore = (await reviews.findOne({ userId, exerciseId: "t/hard" }))!;
  assert.equal(await logRetry(uid, "t/hard", 4, { correct: true, questionIndex: 2 }), false, "wrong question is refused");
  assert.equal(await logRetry(uid, "t/hard", 4, { correct: true, questionIndex: 1 }), true);
  assert.equal(await logRetry(uid, "t/hard", 4, { correct: true, questionIndex: 1 }), false, "second retry of the same miss is refused");
  assert.equal(await logRetry(uid, "t/card", 4, { correct: true, questionIndex: 2 }), false, "no miss, no retry");
  const dueAfter = (await reviews.findOne({ userId, exerciseId: "t/hard" }))!;
  assert.equal(+dueAfter.dueAt, +dueBefore.dueAt); assert.equal(dueAfter.interval, dueBefore.interval);
  assert.equal(await events.countDocuments({ userId, exerciseId: "t/hard", source: "retry" }), 1);
  await seed("t/solo"); await settleReviewCard(uid, "t/solo", { correct: false, questionIndex: 0 });
  assert.equal(await logRetry(uid, "t/solo", 1, { correct: false, questionIndex: 0 }), true, "one-question deck retries the same question");

  // Skip moves dueAt to tomorrow (user's zone), leaves interval, logs once.
  await seed("t/skip", 21);
  const before = (await reviews.findOne({ userId, exerciseId: "t/skip" }))!;
  assert.equal(await skipReview(uid, "t/skip", "Europe/London"), true);
  assert.equal(await skipReview(uid, "t/skip", "Europe/London"), false, "replayed skip is a no-op");
  const skipped = (await reviews.findOne({ userId, exerciseId: "t/skip" }))!;
  assert.equal(+skipped.dueAt, +startOfLocalDay(new Date(), "Europe/London", 1));
  assert.ok(skipped.dueAt > new Date() && skipped.dueAt.getTime() - Date.now() <= DAY + 1000);
  assert.equal(skipped.interval, 21); assert.equal(+skipped.lastAt, +before.lastAt);
  const skipEvents = await events.find({ userId, exerciseId: "t/skip" }).toArray();
  assert.equal(skipEvents.length, 1); assert.equal(skipEvents[0].source, "skip"); assert.equal(skipEvents[0].intervalAfter, 21);
  assert.equal(await reviews.countDocuments({ userId, dueAt: { $lte: new Date() }, exerciseId: "t/skip" }), 0, "due count drops");

  // Coding pass: Easy without hints, Good with; count goes up; one event.
  await seed("t/code");
  assert.equal(await recordReviewPass(uid, "t/code"), true);
  assert.equal(await recordReviewPass(uid, "t/code"), false);
  const code = await events.find({ userId, exerciseId: "t/code" }).toArray();
  assert.equal(code.length, 1); assert.equal(code[0].source, "coding"); assert.equal(code[0].rating, "easy");
  assert.equal((await reviews.findOne({ userId, exerciseId: "t/code" }))!.count, 1);
  await seed("t/code-hint", 7, { hintsInReview: 2 }); await recordReviewPass(uid, "t/code-hint");
  assert.equal((await events.findOne({ userId, exerciseId: "t/code-hint" }))!.rating, "good");
  assert.equal((await reviews.findOne({ userId, exerciseId: "t/code-hint" }))!.hintsInReview, undefined, "hint counter resets");
  console.log("review checks passed");
} finally {
  await db.dropDatabase();
  // Close the shared test client without overriding a failed assertion's exit status.
  const cache = globalThis as unknown as { __mongo?: { client: MongoClient } };
  await cache.__mongo?.client.close();
}
