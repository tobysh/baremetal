import "server-only";
import { ObjectId } from "mongodb";
import { getDb } from "./db";
import { REVIEW_INTERVALS_DAYS } from "./progress-constants";
import { challengeXpFor, xpFor, type Difficulty } from "./gamification";
import type { ProgressDoc, ReviewDoc, TutorThreadDoc, UserDoc, XpEventDoc } from "./models";

const col = async () => (await getDb()).collection<ProgressDoc>("progress");

export async function saveCode(userId: string, exerciseId: string, trackSlug: string, code: string) {
  await (await col()).updateOne(
    { userId: new ObjectId(userId), exerciseId },
    { $set: { code, updatedAt: new Date(), trackSlug }, $setOnInsert: { status: "started", attempts: 0 } },
    { upsert: true },
  );
}

export type AttemptOutcome = { firstPass: boolean; xpAwarded: number; totalXp: number | null; paced?: boolean; unaided?: boolean };

/**
 * Test results come from the learner's own process, so a determined learner can
 * forge them. This caps the damage: more than MAX first passes inside WINDOW_MS
 * still counts as passed, but earns no XP.
 */
export const PASS_PACE = { max: 6, windowMs: 10 * 60 * 1000 };

export async function recordAttempt(o: {
  userId: string;
  exerciseId: string;
  trackSlug: string;
  code: string;
  passed: boolean;
  difficulty: Difficulty;
  /** True for a lesson's optional challenge (half XP, no seals, no review). */
  challenge: boolean;
}): Promise<AttemptOutcome> {
  const now = new Date();
  const c = await col();
  const userId = new ObjectId(o.userId);
  const filter = { userId, exerciseId: o.exerciseId };
  const after = await c.findOneAndUpdate(
    filter,
    { $set: { code: o.code, updatedAt: now, trackSlug: o.trackSlug }, $inc: { attempts: 1 }, $setOnInsert: { status: "started" } },
    { upsert: true, returnDocument: "after", projection: { code: 0 } },
  );
  if (!o.passed) return { firstPass: false, xpAwarded: 0, totalXp: null };

  // Passing is permanent, and XP is granted once: only the request that flips
  // the status to "passed" awards it, even if Test is pressed twice at once.
  const recent = await c.countDocuments({ userId, status: "passed", passedAt: { $gte: new Date(now.getTime() - PASS_PACE.windowMs) } });
  const paced = recent >= PASS_PACE.max;
  const xp = o.challenge ? challengeXpFor(o.difficulty) : xpFor(o.difficulty);
  const award = paced ? 0 : xp;

  // Seals are decided from what the server recorded: hints opened and tutor messages sent before this pass.
  const attempts = after?.attempts ?? 1;
  const hintsUsed = after?.hintsUsed ?? 0;
  let unaided = false;
  if (!o.challenge) {
    const thread = await (await getDb()).collection<TutorThreadDoc>("tutor_threads").findOne({ userId, lessonId: o.exerciseId }, { projection: { messages: 1 } });
    const askedTutor = (thread?.messages ?? []).some((m) => m.role === "user");
    unaided = hintsUsed === 0 && !askedTutor && (attempts >= 2 || o.difficulty === "hard" || o.difficulty === "impossible");
  }

  const flipped = await c.updateOne(
    { ...filter, status: { $ne: "passed" } },
    { $set: { status: "passed", passedAt: now, xpAwarded: award, hintsUsedAtPass: hintsUsed, attemptsAtPass: attempts, unaided, ...(paced ? { xpWithheld: true } : {}) } },
  );
  if (flipped.modifiedCount !== 1) return { firstPass: false, xpAwarded: 0, totalXp: null };
  if (!o.challenge) await scheduleFirstReview(userId, o.exerciseId, now);
  if (paced) return { firstPass: true, xpAwarded: 0, totalXp: null, paced: true, unaided };

  const db = await getDb();
  try {
    await db.collection<XpEventDoc>("xp_events").insertOne({ userId, source: o.challenge ? "challenge" : "lesson", ref: o.exerciseId, xp: award, at: now } as XpEventDoc);
  } catch (e) {
    if ((e as { code?: number }).code === 11000) return { firstPass: false, xpAwarded: 0, totalXp: null }; // already in the ledger
    throw e;
  }
  const user = await db
    .collection<UserDoc>("users")
    .findOneAndUpdate({ _id: userId }, { $inc: { xp: award } }, { returnDocument: "after", projection: { xp: 1 } });
  return { firstPass: true, xpAwarded: award, totalXp: user?.xp ?? award, unaided };
}

/** Records that the learner opened hint number `index` (0-based). Only ever moves the count up. */
export async function recordHint(userId: string, exerciseId: string, trackSlug: string, index: number) {
  await (await col()).updateOne(
    { userId: new ObjectId(userId), exerciseId },
    { $max: { hintsUsed: index + 1 }, $set: { trackSlug }, $setOnInsert: { code: "", status: "started", attempts: 0, updatedAt: new Date() } },
    { upsert: true },
  );
}

// ---------- Reviews (spaced repetition of passed lessons; never any XP) ----------

const DAY = 86_400_000;

async function scheduleFirstReview(userId: ObjectId, exerciseId: string, now: Date) {
  const db = await getDb();
  await db
    .collection<ReviewDoc>("reviews")
    .updateOne({ userId, exerciseId }, { $setOnInsert: { dueAt: new Date(now.getTime() + REVIEW_INTERVALS_DAYS[0] * DAY), interval: REVIEW_INTERVALS_DAYS[0], lastAt: now, count: 0 } }, { upsert: true });
}

/** A passed review moves the next one further out. Only counts when the review was actually due, so it cannot be farmed. */
export async function recordReviewPass(userId: string, exerciseId: string): Promise<boolean> {
  const now = new Date();
  const reviews = (await getDb()).collection<ReviewDoc>("reviews");
  const doc = await reviews.findOne({ userId: new ObjectId(userId), exerciseId });
  if (!doc || doc.dueAt > now) return false;
  const next = REVIEW_INTERVALS_DAYS[Math.min(REVIEW_INTERVALS_DAYS.indexOf(doc.interval) + 1, REVIEW_INTERVALS_DAYS.length - 1)] ?? REVIEW_INTERVALS_DAYS[0];
  const res = await reviews.updateOne(
    { _id: doc._id, dueAt: { $lte: now } },
    { $set: { interval: next, dueAt: new Date(now.getTime() + next * DAY), lastAt: now }, $inc: { count: 1 } },
  );
  return res.modifiedCount === 1;
}

/** "I need a refresher": start the schedule again from the shortest interval. */
export async function resetReview(userId: string, exerciseId: string) {
  const now = new Date();
  await (await getDb())
    .collection<ReviewDoc>("reviews")
    .updateOne({ userId: new ObjectId(userId), exerciseId }, { $set: { interval: REVIEW_INTERVALS_DAYS[0], dueAt: new Date(now.getTime() + REVIEW_INTERVALS_DAYS[0] * DAY), lastAt: now } });
}

export async function getReviewFor(userId: string, exerciseId: string): Promise<ReviewDoc | null> {
  return (await getDb()).collection<ReviewDoc>("reviews").findOne({ userId: new ObjectId(userId), exerciseId });
}

export async function getReviews(userId: string): Promise<ReviewDoc[]> {
  return (await getDb()).collection<ReviewDoc>("reviews").find({ userId: new ObjectId(userId) }).sort({ dueAt: 1 }).toArray();
}

export async function getProgress(userId: string, exerciseId: string) {
  return (await col()).findOne({ userId: new ObjectId(userId), exerciseId });
}

/** exerciseId -> status, for track and dashboard views. */
export async function getProgressMap(userId: string): Promise<Map<string, ProgressDoc>> {
  const docs = await (await col()).find({ userId: new ObjectId(userId) }, { projection: { code: 0 } }).toArray();
  return new Map(docs.map((d) => [d.exerciseId, d]));
}

export async function getTutorThread(userId: string, lessonId: string) {
  const db = await getDb();
  return db.collection<TutorThreadDoc>("tutor_threads").findOne({ userId: new ObjectId(userId), lessonId });
}
