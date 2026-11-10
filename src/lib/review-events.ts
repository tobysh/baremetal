import "server-only";
import type { ObjectId } from "mongodb";
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

/** Appends one row to the review event log. Callers write only after the schedule actually moved. */
export async function logReviewEvent(e: EventInput): Promise<ObjectId> {
  const doc: Omit<ReviewEventDoc, "_id"> = {
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
  const res = await (await getDb()).collection<Omit<ReviewEventDoc, "_id">>("review_events").insertOne(doc);
  return res.insertedId;
}
