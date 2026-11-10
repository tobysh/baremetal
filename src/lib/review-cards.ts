import "server-only";
import { ObjectId } from "mongodb";
import { getDb } from "./db";
import type { Lesson } from "./curriculum/types";
import type { ReviewDoc, ReviewEventDoc } from "./models";
import { REVIEW_INTERVALS_DAYS } from "./progress-constants";
import { cardRating, logReviewEvent } from "./review-events";
import { DAY, dueAfter, startOfLocalDay } from "./review-math";


/** A lesson's recall questions: its practice exercise first, then anything in quiz.json. Empty when the lesson has neither. */
export const reviewDeck = (lesson: Pick<Lesson, "practice" | "quiz">) => [...(lesson.practice ? [lesson.practice] : []), ...(lesson.quiz ?? [])];

/** Which question a due lesson shows now. It changes every time the lesson comes round again, so a repeat is not the same card. */
/** A different question from the same deck, for the retry after a miss. Null when the deck has only one question. */
export const retryQuestion = (deckSize: number, missed: number) => (deckSize < 2 ? null : (missed + 1) % deckSize);

export const pickQuestion = (deckSize: number, doc: Pick<ReviewDoc, "lastAt" | "interval">) =>
  deckSize === 0 ? -1 : (Math.floor(new Date(doc.lastAt).getTime() / DAY) + doc.interval) % deckSize;

export type CardOutcome = { correct: boolean; partial?: { right: number; total: number }; questionIndex: number };

/**
 * Settles a due review from a graded card answer. Right moves the lesson to its next interval, wrong starts the
 * schedule over. A card never adds to `count`, so it never counts toward Codex review achievements, and it only
 * acts on a review that is due, which also stops a replayed request from moving the schedule twice.
 * Returns the id of the logged event when the schedule moved, otherwise null.
 */
export async function settleReviewCard(userId: string, exerciseId: string, outcome: CardOutcome): Promise<ObjectId | null> {
  const now = new Date();
  const reviews = (await getDb()).collection<ReviewDoc>("reviews");
  const doc = await reviews.findOne({ userId: new ObjectId(userId), exerciseId });
  if (!doc || doc.dueAt > now) return null;
  const next = outcome.correct
    ? REVIEW_INTERVALS_DAYS[Math.min(REVIEW_INTERVALS_DAYS.indexOf(doc.interval) + 1, REVIEW_INTERVALS_DAYS.length - 1)] ?? REVIEW_INTERVALS_DAYS[0]
    : REVIEW_INTERVALS_DAYS[0];
  const res = await reviews.updateOne({ _id: doc._id, dueAt: { $lte: now } }, { $set: { interval: next, dueAt: dueAfter(now, next), lastAt: now } });
  if (res.modifiedCount !== 1) return null;
  return logReviewEvent({
    userId: doc.userId, exerciseId, source: "card", before: doc, intervalAfter: next, now,
    correct: outcome.correct, partial: outcome.partial, rating: cardRating(outcome.correct, outcome.partial), questionIndex: outcome.questionIndex,
  });
}

/**
 * "Skip for now" on a lesson with no question deck: the review stays due but not today. It moves to the start of
 * tomorrow in the learner's timezone and the interval is untouched. Guarded by `dueAt <= now`, so a replay is a no-op.
 */
export async function skipReview(userId: string, exerciseId: string, timeZone: string | null | undefined): Promise<boolean> {
  const now = new Date();
  const reviews = (await getDb()).collection<ReviewDoc>("reviews");
  const doc = await reviews.findOne({ userId: new ObjectId(userId), exerciseId });
  if (!doc || doc.dueAt > now) return false;
  const res = await reviews.updateOne({ _id: doc._id, dueAt: { $lte: now } }, { $set: { dueAt: startOfLocalDay(now, timeZone ?? undefined, 1) } });
  if (res.modifiedCount !== 1) return false;
  await logReviewEvent({ userId: doc.userId, exerciseId, source: "skip", before: doc, intervalAfter: doc.interval, now, correct: null });
  return true;
}

/** How long after a missed card its retry can still be answered. */
const RETRY_WINDOW_MS = 2 * 60 * 60 * 1000;

/**
 * Logs a retry attempt after a missed card. It never touches the schedule. A retry is only accepted for a card miss
 * from the last two hours that has no retry yet; the unique `retryOf` index makes "once per miss" atomic.
 * Returns false when there is no miss to retry or it was already retried.
 */
export async function logRetry(userId: string, exerciseId: string, deckSize: number, outcome: CardOutcome): Promise<boolean> {
  const now = new Date();
  const db = await getDb();
  const uid = new ObjectId(userId);
  const miss = await db.collection<ReviewEventDoc>("review_events").findOne(
    { userId: uid, exerciseId, source: "card", correct: false, at: { $gte: new Date(now.getTime() - RETRY_WINDOW_MS) } },
    { sort: { at: -1 } },
  );
  if (!miss) return false;
  // Only the question the miss earned: the next one in the deck, or the same one when the deck has just one.
  if (outcome.questionIndex !== (retryQuestion(deckSize, miss.questionIndex ?? 0) ?? miss.questionIndex)) return false;
  const doc = await db.collection<ReviewDoc>("reviews").findOne({ userId: uid, exerciseId });
  if (!doc) return false;
  try {
    await logReviewEvent({
      userId: uid, exerciseId, source: "retry", before: { dueAt: miss.at, lastAt: miss.at, interval: doc.interval }, intervalAfter: doc.interval, now,
      correct: outcome.correct, partial: outcome.partial, questionIndex: outcome.questionIndex, retryOf: miss._id,
    });
  } catch (e) {
    if ((e as { code?: number }).code === 11000) return false;
    throw e;
  }
  return true;
}

/** True when this lesson has a review due right now. */
export async function isReviewDue(userId: string, exerciseId: string): Promise<boolean> {
  const doc = await (await getDb()).collection<ReviewDoc>("reviews").findOne({ userId: new ObjectId(userId), exerciseId }, { projection: { dueAt: 1 } });
  return !!doc && doc.dueAt <= new Date();
}
