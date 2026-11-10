import "server-only";
import { ObjectId } from "mongodb";

import { getDb } from "./db";
import { REVIEW_INTERVALS_DAYS } from "./progress-constants";
import { dueAfter } from "./review-math";
import { codingRating, logReviewEvent } from "./review-events";
import { challengeXpFor, isChallengeId, isPracticeId, practiceXpFor, xpFor, type Difficulty } from "./gamification";
import type { ProgressDoc, ReviewDoc, TutorThreadDoc, XpEventDoc } from "./models";

const col = async () => (await getDb()).collection<ProgressDoc>("progress");

/**
 * When a lesson's starter changes shape (its `revision` goes up), code saved against the older
 * shape must not become the new starting point. The first write at the new revision moves the
 * old code to `previousCode`; later writes leave it alone. Lessons still at revision 1 skip this.
 */
async function archiveOlderRevision(userId: string, exerciseId: string, rev: number) {
  if (rev <= 1) return;
  await (await col()).updateOne(
    { userId: new ObjectId(userId), exerciseId, $or: [{ rev: { $exists: false } }, { rev: { $lt: rev } }] },
    [{ $set: { previousCode: { $ifNull: ["$code", ""] }, rev } }],
  );
}

export async function saveCode(userId: string, exerciseId: string, trackSlug: string, code: string, rev = 1) {
  await archiveOlderRevision(userId, exerciseId, rev);
  await (await col()).updateOne(
    { userId: new ObjectId(userId), exerciseId },
    { $set: { code, updatedAt: new Date(), trackSlug, ...(rev > 1 ? { rev } : {}) }, $setOnInsert: { status: "started", attempts: 0, startedAt: new Date(), startKnown: true } },
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
  challenge?: boolean;
  /** The lesson revision the code was written against (see saveCode). */
  rev?: number;
  /** True for a lesson's tap-and-drag practice (a quarter of the XP, no seals, no review, no Codex credit). The exerciseId is the practice id. */
  practice?: boolean;
};

/** Practice is quick to read and tap, so its too-fast floor is flat and short. */
export const PRACTICE_MIN_SOLVE_MS = 4_000;

/** A claimed first pass is recorded before its atomic per-user XP decision.
 * A crash between those writes leaves xpPending for administrative reconciliation. */
export async function recordAttempt(o: AttemptInput): Promise<AttemptOutcome> {
  const now = new Date();
  const c = await col();
  const userId = new ObjectId(o.userId);
  const filter = { userId, exerciseId: o.exerciseId };
  await archiveOlderRevision(o.userId, o.exerciseId, o.rev ?? 1);
  const attemptUpdate = await c.findOneAndUpdate(
    filter,
    { $set: { code: o.code, updatedAt: now, trackSlug: o.trackSlug, ...(o.rev && o.rev > 1 ? { rev: o.rev } : {}) }, $inc: { attempts: 1 }, $setOnInsert: { status: "started", startedAt: now, startKnown: false } },
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
  if (!o.challenge && !o.practice) {
    const thread = await db.collection<TutorThreadDoc>("tutor_threads").findOne({ userId, lessonId: o.exerciseId }, { projection: { messages: 1 } });
    const askedTutor = after?.tutorUsed || (thread?.messages ?? []).some((m) => m.role === "user");
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

  const xp = o.practice ? practiceXpFor(o.difficulty) : o.challenge ? challengeXpFor(o.difficulty) : xpFor(o.difficulty);
  // A first submission may be the first server contact after solving locally.
  // Only apply the floor when a previous save, hint or attempt establishes a start.
  const knownStart = after?.startedAt && after.startKnown !== false && attemptUpdate.lastErrorObject?.updatedExisting === true;
  const tooFast = !!knownStart && now.getTime() - after!.startedAt!.getTime() < (o.practice ? PRACTICE_MIN_SOLVE_MS : MIN_SOLVE_MS[o.difficulty]);
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
    await db.collection<XpEventDoc>("xp_events").insertOne({ userId, source: o.practice ? "practice" : o.challenge ? "challenge" : "lesson", ref: o.exerciseId, xp: award, at: now } as XpEventDoc);
  }
  if (!o.challenge && !o.practice) await scheduleFirstReview(userId, o.exerciseId, now);
  await c.updateOne(filter, { $set: { xpAwarded: award, ...(flag ? { xpWithheld: true, xpFlag: flag } : {}) }, $unset: { xpPending: "" } });
  return { firstPass: true, xpAwarded: award, totalXp: award ? updated.xp ?? award : null, ...(flag ? { paced: true } : {}), unaided };
}

export type PendingXpReport = { finished: number; reverted: number; unresolved: number };

/**
 * Settles first passes that crashed between claiming the pass and recording the XP decision
 * (`xpPending`). Standalone MongoDB has no transactions, so recordAttempt is a sequence of
 * idempotent steps and this finishes whichever were missed. For each pass older than `olderThanMs`:
 *  - the account's `xpQuota` (24 h) or the ledger already holds the decision: the award was applied
 *    to `users.xp`, so only the ledger row, the first review and the progress fields are completed;
 *  - no decision and the pass is under 24 h old: nothing was awarded, so the claim is reverted and
 *    the learner's next passing run awards it normally;
 *  - no decision and older than the quota window: left pending and counted as unresolved, because
 *    the outcome can no longer be told (`npm run flagged-accounts` lists these).
 */
export async function reconcilePendingXp(olderThanMs = 10 * 60_000): Promise<PendingXpReport> {
  const report: PendingXpReport = { finished: 0, reverted: 0, unresolved: 0 };
  const db = await getDb();
  const c = await col();
  const now = Date.now();
  const stale = c.find({ status: "passed", xpPending: true, passedAt: { $lt: new Date(now - olderThanMs) } }).project({ code: 0 });
  for await (const p of stale) {
    const users = db.collection("users");
    const events = db.collection<XpEventDoc>("xp_events");
    const user = await users.findOne({ _id: p.userId }, { projection: { xpQuota: 1 } });
    const decision = (user?.xpQuota ?? []).find((e: { ref?: string }) => e.ref === p.exerciseId) as { xp?: number; flag?: XpFlag | null } | undefined;
    const source = isPracticeId(p.exerciseId) ? "practice" : isChallengeId(p.exerciseId) ? "challenge" : "lesson";
    const event = await events.findOne({ userId: p.userId, source, ref: p.exerciseId });
    if (decision || event) {
      const award = decision ? decision.xp ?? 0 : event!.xp;
      const flag = decision?.flag ?? null;
      if (award > 0 && !event) {
        await events.updateOne({ userId: p.userId, source, ref: p.exerciseId }, { $setOnInsert: { xp: award, at: p.passedAt ?? new Date() } }, { upsert: true });
      }
      if (source === "lesson") await scheduleFirstReview(p.userId, p.exerciseId, p.passedAt ?? new Date());
      await c.updateOne({ _id: p._id, xpPending: true }, { $set: { xpAwarded: award, ...(flag ? { xpWithheld: true, xpFlag: flag } : {}) }, $unset: { xpPending: "" } });
      report.finished++;
    } else if (p.passedAt && now - p.passedAt.getTime() < 24 * 3_600_000) {
      const res = await c.updateOne({ _id: p._id, xpPending: true }, { $set: { status: "started", startKnown: true }, $unset: { xpPending: "", passedAt: "", xpAwarded: "" } });
      if (res.modifiedCount === 1) report.reverted++;
    } else {
      report.unresolved++;
    }
  }
  if (report.unresolved) console.error(`[xp] ${report.unresolved} pending pass(es) older than 24 h have no recorded decision; review them with npm run flagged-accounts.`);
  return report;
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

async function scheduleFirstReview(userId: ObjectId, exerciseId: string, now: Date) {
  const db = await getDb();
  await db
    .collection<ReviewDoc>("reviews")
    .updateOne({ userId, exerciseId }, { $setOnInsert: { dueAt: dueAfter(now, REVIEW_INTERVALS_DAYS[0]), interval: REVIEW_INTERVALS_DAYS[0], lastAt: now, count: 0 } }, { upsert: true });
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
    { $set: { interval: next, dueAt: dueAfter(now, next), lastAt: now }, $inc: { count: 1 }, $unset: { hintsInReview: "" } },
  );
  if (res.modifiedCount !== 1) return false;
  await logReviewEvent({ userId: doc.userId, exerciseId, source: "coding", before: doc, intervalAfter: next, now, correct: true, rating: codingRating(doc.hintsInReview) });
  return true;
}

/** "I need a refresher": start the schedule again from the shortest interval. */
export async function resetReview(userId: string, exerciseId: string) {
  const now = new Date();
  await (await getDb())
    .collection<ReviewDoc>("reviews")
    .updateOne({ userId: new ObjectId(userId), exerciseId }, { $set: { interval: REVIEW_INTERVALS_DAYS[0], dueAt: dueAfter(now, REVIEW_INTERVALS_DAYS[0]), lastAt: now }, $unset: { hintsInReview: "" } });
}

/** Notes that the learner opened hint `index` (0-based) during a coding review. Only a review that exists is touched. */
export async function recordReviewHint(userId: string, exerciseId: string, index: number) {
  await (await getDb()).collection<ReviewDoc>("reviews").updateOne({ userId: new ObjectId(userId), exerciseId }, { $max: { hintsInReview: index + 1 } });
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
