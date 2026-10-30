import "server-only";
import { getDb } from "./db";
import { getTracks } from "./curriculum";
import { buildLearningCohorts, type LearnerCourse } from "./learning-cohorts";

export async function getLearningReport() {
  const tracks = getTracks();
  const db = await getDb();
  const groups = await db.collection("progress").aggregate<LearnerCourse>([
    { $match: { exerciseId: { $in: tracks.flatMap((track) => track.lessons.map((lesson) => lesson.exercise.id)) } } },
    { $group: {
      _id: { userId: "$userId", trackSlug: "$trackSlug" },
      lessons: { $push: { exerciseId: "$exerciseId", startedAt: "$startedAt", passedAt: "$passedAt", status: "$status" } },
    } },
    { $project: { _id: 0, trackSlug: "$_id.trackSlug", lessons: 1 } },
  ], { maxTimeMS: 15_000, allowDiskUse: true }).toArray();
  return buildLearningCohorts(tracks, groups);
}
