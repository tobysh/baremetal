import "server-only";
import { ObjectId, type Db, type UpdateFilter } from "mongodb";
import { getDb } from "./db";
import { DAY } from "./review-math";
import type { ReviewDoc, ReviewEventDoc, ReviewRating } from "./models";

type EventInput = {
  userId: ObjectId;
  exerciseId: string;
  source: ReviewEventDoc["source"];
  /** The review doc as it was before this change. */
  before: Pick<ReviewDoc, "dueAt" | "lastAt" | "interval">;
  intervalAfter: number;
  now: Date;
  correct: boolean | null;
  rating?: ReviewRating;
  partial?: { right: number; total: number };
  questionIndex?: number;
  retryOf?: ObjectId;
};

/** Rating a graded card answer: wrong is Again, partly right is Hard, right is Good. */
export const cardRating = (correct: boolean, partial?: { right: number; total: number }): ReviewRating =>
  correct ? "good" : partial && partial.right > 0 ? "hard" : "again";

/** Rating a passed coding re-solve: no hints opened during the review is Easy, otherwise Good. */
export const codingRating = (hintsInReview: number | undefined): ReviewRating => ((hintsInReview ?? 0) === 0 ? "easy" : "good");

/** Build the full event before changing its schedule, including its stable recovery key. */
export function reviewEvent(e: EventInput): ReviewEventDoc {
  return {
    _id: new ObjectId(),
    userId: e.userId,
    exerciseId: e.exerciseId,
    at: e.now,
    source: e.source,
    ...(e.questionIndex !== undefined ? { questionIndex: e.questionIndex } : {}),
    correct: e.correct,
    ...(e.partial ? { partial: `${e.partial.right} of ${e.partial.total}` } : {}),
    ...(e.rating ? { rating: e.rating } : {}),
    daysLate: (e.now.getTime() - e.before.dueAt.getTime()) / DAY,
    elapsedDays: (e.now.getTime() - e.before.lastAt.getTime()) / DAY,
    intervalBefore: e.before.interval,
    intervalAfter: e.intervalAfter,
    ...(e.retryOf ? { retryOf: e.retryOf } : {}),
  };
}

/** Appends a retry event, which does not change a schedule. */
export async function logReviewEvent(e: EventInput): Promise<ObjectId> {
  const res = await (await getDb()).collection<ReviewEventDoc>("review_events").insertOne(reviewEvent(e));
  return res.insertedId;
}

/** Replay is safe even when insertion succeeded but its response or acknowledgement was lost. */
export async function flushPendingReviewEvent(db: Db, doc: ReviewDoc): Promise<void> {
  const event = doc.pendingEvent;
  if (!event) return;
  await db.collection<ReviewEventDoc>("review_events").updateOne({ _id: event._id }, { $setOnInsert: event }, { upsert: true });
  // Account deletion removes users before its history cleanup. A recovery cursor
  // can outlive that cleanup, so check after insertion to prevent resurrection.
  if (!await db.collection("users").findOne({ _id: event.userId }, { projection: { _id: 1 } })) {
    await db.collection<ReviewEventDoc>("review_events").deleteOne({ _id: event._id });
  }
  // Never acknowledge a later event installed by another request.
  await db.collection<ReviewDoc>("reviews").updateOne(
    { _id: doc._id, "pendingEvent._id": event._id },
    { $unset: { pendingEvent: "" } },
  );
}

/** Recover the last schedule change before checking whether another answer is due. */
export async function reviewWithRecoveredEvent(db: Db, userId: ObjectId, exerciseId: string): Promise<ReviewDoc | null> {
  const doc = await db.collection<ReviewDoc>("reviews").findOne({ userId, exerciseId });
  if (doc?.pendingEvent) await flushPendingReviewEvent(db, doc);
  return doc;
}

/** One-document outbox works on standalone MongoDB as well as replica sets. */
export async function updateReviewWithEvent(db: Db, before: ReviewDoc, event: ReviewEventDoc, update: UpdateFilter<ReviewDoc>): Promise<boolean> {
  const result = await db.collection<ReviewDoc>("reviews").updateOne(
    { _id: before._id, dueAt: before.dueAt, lastAt: before.lastAt, interval: before.interval, pendingEvent: { $exists: false } },
    { ...update, $set: { ...update.$set, pendingEvent: event } },
  );
  if (result.modifiedCount !== 1) return false;
  await flushPendingReviewEvent(db, { ...before, pendingEvent: event });
  return true;
}

/** Startup/periodic recovery; failures remain durable and are reported for operational alerts. */
export async function reconcilePendingReviewEvents(): Promise<{ recovered: number; failed: number }> {
  const db = await getDb();
  const report = { recovered: 0, failed: 0 };
  for await (const doc of db.collection<ReviewDoc>("reviews").find({ pendingEvent: { $exists: true } })) {
    try {
      await flushPendingReviewEvent(db, doc);
      report.recovered++;
    } catch (error) {
      report.failed++;
      console.error("[review] pending event recovery failed; event retained for retry", error);
    }
  }
  return report;
}
