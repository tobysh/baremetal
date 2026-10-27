import "server-only";
import { ObjectId } from "mongodb";
import { getDb } from "./db";
import type { ProgressDoc, TutorThreadDoc, UserDoc } from "./models";

const col = async () => (await getDb()).collection<ProgressDoc>("progress");

export async function saveCode(userId: string, exerciseId: string, trackSlug: string, code: string) {
  await (await col()).updateOne(
    { userId: new ObjectId(userId), exerciseId },
    { $set: { code, updatedAt: new Date(), trackSlug }, $setOnInsert: { status: "started", attempts: 0 } },
    { upsert: true },
  );
}

export type AttemptOutcome = { firstPass: boolean; xpAwarded: number; totalXp: number | null; paced?: boolean };

/**
 * Test results come from the learner's own process, so a determined learner can
 * forge them. This caps the damage: more than MAX first passes inside WINDOW_MS
 * still counts as passed, but earns no XP.
 */
export const PASS_PACE = { max: 6, windowMs: 10 * 60 * 1000 };

export async function recordAttempt(
  userId: string,
  exerciseId: string,
  trackSlug: string,
  code: string,
  passed: boolean,
  xp: number,
): Promise<AttemptOutcome> {
  const now = new Date();
  const c = await col();
  const filter = { userId: new ObjectId(userId), exerciseId };
  await c.updateOne(
    filter,
    { $set: { code, updatedAt: now, trackSlug }, $inc: { attempts: 1 }, $setOnInsert: { status: "started" } },
    { upsert: true },
  );
  if (!passed) return { firstPass: false, xpAwarded: 0, totalXp: null };
  // Passing is permanent, and XP is granted once: only the request that flips
  // the status to "passed" awards it, even if Test is pressed twice at once.
  const recent = await c.countDocuments({ userId: filter.userId, status: "passed", passedAt: { $gte: new Date(now.getTime() - PASS_PACE.windowMs) } });
  const paced = recent >= PASS_PACE.max;
  const award = paced ? 0 : xp;
  const flipped = await c.updateOne(
    { ...filter, status: { $ne: "passed" } },
    { $set: { status: "passed", passedAt: now, xpAwarded: award, ...(paced ? { xpWithheld: true } : {}) } },
  );
  if (flipped.modifiedCount !== 1) return { firstPass: false, xpAwarded: 0, totalXp: null };
  if (paced) return { firstPass: true, xpAwarded: 0, totalXp: null, paced: true };
  const db = await getDb();
  const user = await db
    .collection<UserDoc>("users")
    .findOneAndUpdate({ _id: new ObjectId(userId) }, { $inc: { xp } }, { returnDocument: "after", projection: { xp: 1 } });
  return { firstPass: true, xpAwarded: xp, totalXp: user?.xp ?? xp };
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
