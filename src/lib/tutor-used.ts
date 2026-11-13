import type { Db } from "mongodb";
import { ObjectId } from "mongodb";

/** Sticky flag that costs the learner the Unaided seal. Set when a tutor call starts, so an aborted reply still counts. */
export async function markTutorUsed(db: Db, userId: string, exercise: { id: string; trackSlug: string }, code: string): Promise<void> {
  const now = new Date();
  await db.collection("progress").updateOne(
    { userId: new ObjectId(userId), exerciseId: exercise.id },
    { $set: { tutorUsed: true, updatedAt: now }, $setOnInsert: { trackSlug: exercise.trackSlug, code, status: "started", attempts: 0, startedAt: now, startKnown: false } },
    { upsert: true },
  );
}
