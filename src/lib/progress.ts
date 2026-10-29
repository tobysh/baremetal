import "server-only";
import { ObjectId } from "mongodb";

import { getDb } from "./db";
import { REVIEW_INTERVALS_DAYS } from "./progress-constants";
import { challengeXpFor, xpFor, type Difficulty } from "./gamification";
import type { ProgressDoc, ReviewDoc, TutorThreadDoc, XpEventDoc } from "./models";

const col = async () => (await getDb()).collection<ProgressDoc>("progress");

export async function saveCode(userId: string, exerciseId: string, trackSlug: string, code: string) {
  await (await col()).updateOne(
    { userId: new ObjectId(userId), exerciseId },
    { $set: { code, updatedAt: new Date(), trackSlug }, $setOnInsert: { status: "started", attempts: 0, startedAt: new Date(), startKnown: true } },
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

/** First-pass XP over any rolling 24 hours; further passes still count as passed but earn nothing. */
export const DAILY_XP_CAP = 400;

/** A first pass sooner than this after the learner first touched the exercise is withheld and flagged. */
export const MIN_SOLVE_MS: Record<Difficulty, number> = { easy: 15_000, medium: 30_000, hard: 60_000, impossible: 120_000 };

export type XpFlag = "pace" | "daily-cap" | "too-fast";

type AttemptInput = {
  userId: string;
  exerciseId: string;
  trackSlug: string;
  code: string;
  passed: boolean;
  difficulty: Difficulty;
  /** True for a lesson's optional challenge (half XP, no seals, no review). */
  challenge: boolean;
};

/** A claimed first pass is recorded before its atomic per-user XP decision.
 * A crash between those writes leaves xpPending for administrative reconciliation. */
export async function recordAttempt(o: AttemptInput): Promise<AttemptOutcome> {
  const now = new Date();
  const c = await col();
  const userId = new ObjectId(o.userId);
  const filter = { userId, exerciseId: o.exerciseId };
  const attemptUpdate = await c.findOneAndUpdate(
    filter,
    { $set: { code: o.code, updatedAt: now, trackSlug: o.trackSlug }, $inc: { attempts: 1 }, $setOnInsert: { status: "started", startedAt: now, startKnown: false } },
    { upsert: true, returnDocument: "after", projection: { code: 0 }, includeResultMetadata: true },
  );
  const after = attemptUpdate.value;
  if (!o.passed) {
    await c.updateOne({ ...filter, startKnown: false }, { $set: { startKnown: true } });
    return { firstPass: false, xpAwarded: 0, totalXp: null };
  }

  const attempts = after?.attempts ?? 1, hintsUsed = after?.hintsUsed ?? 0;
  let unaided = false;
  const db = await getDb();
  if (!o.challenge) {
    const thread = await db.collection<TutorThreadDoc>("tutor_threads").findOne({ userId, lessonId: o.exerciseId }, { projection: { messages: 1 } });
    const askedTutor = (thread?.messages ?? []).some((m) => m.role === "user");
    unaided = hintsUsed === 0 && !askedTutor && (attempts >= 2 || o.difficulty === "hard" || o.difficulty === "impossible");
  }
  const flipped = await c.updateOne(
    { ...filter, status: { $ne: "passed" } },
    { $set: { status: "passed", passedAt: now, xpAwarded: 0, xpPending: true, hintsUsedAtPass: hintsUsed, attemptsAtPass: attempts, unaided } },
  );
  if (flipped.modifiedCount !== 1) return { firstPass: false, xpAwarded: 0, totalXp: null };

  // Seed existing accounts once from their last 24 hours of completed decisions.
  // Pending passes are omitted; their own atomic quota updates will append them.
  const users = db.collection("users");
  const dayStart = new Date(now.getTime() - 24 * 3_600_000);
  const history = await c.find({ userId, status: "passed", xpPending: { $ne: true }, passedAt: { $gte: dayStart } }, { projection: { exerciseId: 1, passedAt: 1, xpAwarded: 1 } }).toArray();
  await users.updateOne({ _id: userId, xpQuota: { $exists: false } }, { $set: { xpQuota: history.map((p) => ({ ref: p.exerciseId, at: p.passedAt, xp: p.xpAwarded ?? 0 })) } });

  const xp = o.challenge ? challengeXpFor(o.difficulty) : xpFor(o.difficulty);
  // A first submission may be the first server contact after solving locally.
  // Only apply the floor when a previous save, hint or attempt establishes a start.
  const knownStart = after?.startedAt && after.startKnown !== false && attemptUpdate.lastErrorObject?.updatedExisting === true;
  const tooFast = !!knownStart && now.getTime() - after!.startedAt!.getTime() < MIN_SOLVE_MS[o.difficulty];
  const updated = await users.findOneAndUpdate({ _id: userId }, [
    { $set: { xpQuota: { $filter: { input: { $ifNull: ["$xpQuota", []] }, as: "entry", cond: { $gte: ["$$entry.at", dayStart] } } } } },
    { $set: { xpDecision: { $let: { vars: {
      recent: { $size: { $filter: { input: "$xpQuota", as: "entry", cond: { $gte: ["$$entry.at", new Date(now.getTime() - PASS_PACE.windowMs)] } } } },
      total: { $sum: "$xpQuota.xp" },
    }, in: { flag: { $cond: [ { $gte: ["$$recent", PASS_PACE.max] }, "pace", { $cond: [tooFast, "too-fast", { $cond: [{ $gt: [{ $add: ["$$total", xp] }, DAILY_XP_CAP] }, "daily-cap", null] }] } ] } } } } } },
    { $set: { "xpDecision.award": { $cond: [{ $eq: ["$xpDecision.flag", null] }, xp, 0] } } },
    { $set: { xpQuota: { $concatArrays: ["$xpQuota", [{ ref: { $literal: o.exerciseId }, at: now, xp: "$xpDecision.award", flag: "$xpDecision.flag" }]] }, xp: { $add: [{ $ifNull: ["$xp", 0] }, "$xpDecision.award"] } } },
  ], { returnDocument: "after", projection: { xp: 1, xpDecision: 1 } });
  if (!updated) throw new Error("User disappeared while awarding XP; the pending pass needs reconciliation.");
  const flag = updated.xpDecision.flag as XpFlag | null, award = updated.xpDecision.award as number;
  if (award) {
    await db.collection<XpEventDoc>("xp_events").insertOne({ userId, source: o.challenge ? "challenge" : "lesson", ref: o.exerciseId, xp: award, at: now } as XpEventDoc);
  }
  if (!o.challenge) await scheduleFirstReview(userId, o.exerciseId, now);
  await c.updateOne(filter, { $set: { xpAwarded: award, ...(flag ? { xpWithheld: true, xpFlag: flag } : {}) }, $unset: { xpPending: "" } });
  return { firstPass: true, xpAwarded: award, totalXp: award ? updated.xp ?? award : null, ...(flag ? { paced: true } : {}), unaided };
}

/** Records that the learner opened hint number `index` (0-based). Only ever moves the count up. */
export async function recordHint(userId: string, exerciseId: string, trackSlug: string, index: number) {
  await (await col()).updateOne(
    { userId: new ObjectId(userId), exerciseId },
    { $max: { hintsUsed: index + 1 }, $set: { trackSlug }, $setOnInsert: { code: "", status: "started", attempts: 0, updatedAt: new Date(), startedAt: new Date(), startKnown: true } },
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
