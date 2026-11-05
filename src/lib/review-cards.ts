import "server-only";
import { ObjectId } from "mongodb";
import { getDb } from "./db";
import type { Lesson } from "./curriculum/types";
import type { ReviewDoc } from "./models";
import { REVIEW_INTERVALS_DAYS } from "./progress-constants";

const DAY = 86_400_000;

/** A lesson's recall questions: its practice exercise first, then anything in quiz.json. Empty when the lesson has neither. */
export const reviewDeck = (lesson: Pick<Lesson, "practice" | "quiz">) => [...(lesson.practice ? [lesson.practice] : []), ...(lesson.quiz ?? [])];

/** Which question a due lesson shows now. It changes every time the lesson comes round again, so a repeat is not the same card. */
export const pickQuestion = (deckSize: number, doc: Pick<ReviewDoc, "lastAt" | "interval">) =>
  deckSize === 0 ? -1 : (Math.floor(new Date(doc.lastAt).getTime() / DAY) + doc.interval) % deckSize;

/**
 * Settles a due review from a graded card answer. Right moves the lesson to its next interval, wrong starts the
 * schedule over. A card never adds to `count`, so it never counts toward Codex review achievements, and it only
 * acts on a review that is due, which also stops a replayed request from moving the schedule twice.
 * Returns whether the schedule moved.
 */
export async function settleReviewCard(userId: string, exerciseId: string, correct: boolean): Promise<boolean> {
  const now = new Date();
  const reviews = (await getDb()).collection<ReviewDoc>("reviews");
  const doc = await reviews.findOne({ userId: new ObjectId(userId), exerciseId });
  if (!doc || doc.dueAt > now) return false;
  const next = correct
    ? REVIEW_INTERVALS_DAYS[Math.min(REVIEW_INTERVALS_DAYS.indexOf(doc.interval) + 1, REVIEW_INTERVALS_DAYS.length - 1)] ?? REVIEW_INTERVALS_DAYS[0]
    : REVIEW_INTERVALS_DAYS[0];
  const res = await reviews.updateOne({ _id: doc._id, dueAt: { $lte: now } }, { $set: { interval: next, dueAt: new Date(now.getTime() + next * DAY), lastAt: now } });
  return res.modifiedCount === 1;
}

/** True when this lesson has a review due right now. */
export async function isReviewDue(userId: string, exerciseId: string): Promise<boolean> {
  const doc = await (await getDb()).collection<ReviewDoc>("reviews").findOne({ userId: new ObjectId(userId), exerciseId }, { projection: { dueAt: 1 } });
  return !!doc && doc.dueAt <= new Date();
}
