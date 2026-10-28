import "server-only";
import { ObjectId } from "mongodb";
import { ACHIEVEMENTS, achievementById, type CodexContext } from "./achievements";
import { getTracks } from "./curriculum";
import type { Track } from "./curriculum/types";
import { getDb } from "./db";
import type { AchievementDoc, ProgressDoc, ReviewDoc, UserDoc } from "./models";
import { buildContext } from "./rewards-context";

export async function loadContext(userId: string): Promise<CodexContext> {
  const db = await getDb();
  const _id = new ObjectId(userId);
  const [progress, reviews, user] = await Promise.all([
    db.collection<ProgressDoc>("progress").find({ userId: _id }, { projection: { code: 0 } }).toArray(),
    db.collection<ReviewDoc>("reviews").find({ userId: _id }).toArray(),
    db.collection<UserDoc>("users").findOne({ _id }, { projection: { xp: 1, timezone: 1, weeklyGoal: 1 } }),
  ]);
  return buildContext(getTracks(), progress, reviews, user ?? {});
}

export type Unlocked = { id: string; name: string; description: string };

/** Records any achievements the learner has newly earned and returns them (for the toast). */
export async function evaluateAchievements(userId: string): Promise<{ unlocked: Unlocked[]; ctx: CodexContext }> {
  const db = await getDb();
  const _id = new ObjectId(userId);
  const [ctx, have] = await Promise.all([
    loadContext(userId),
    db.collection<AchievementDoc>("achievements").find({ userId: _id }, { projection: { id: 1 } }).toArray(),
  ]);
  const owned = new Set(have.map((a) => a.id));
  const unlocked: Unlocked[] = [];
  for (const a of ACHIEVEMENTS) {
    if (owned.has(a.id) || !a.check(ctx)) continue;
    try {
      await db.collection("achievements").insertOne({ userId: _id, id: a.id, unlockedAt: new Date() });
      unlocked.push({ id: a.id, name: a.name, description: a.description });
    } catch (e) {
      if ((e as { code?: number }).code !== 11000) throw e; // someone else just recorded it
    }
  }
  return { unlocked, ctx };
}

export async function getUnlocked(userId: string): Promise<{ id: string; unlockedAt: Date }[]> {
  const rows = await (await getDb()).collection<AchievementDoc>("achievements").find({ userId: new ObjectId(userId) }).sort({ unlockedAt: -1 }).toArray();
  return rows.filter((r) => achievementById.has(r.id)).map((r) => ({ id: r.id, unlockedAt: r.unlockedAt }));
}

/** Set when a first pass finishes a whole course: the moment the UI celebrates. */
export type TrackComplete = { slug: string; title: string; lessons: number; unaided: number; challenges: number; next: { slug: string; title: string } | null };

export function trackCompleteFor(trackSlug: string, ctx: CodexContext): TrackComplete | null {
  const tracks = getTracks();
  const track = tracks.find((t) => t.slug === trackSlug);
  if (!track || !track.lessons.every((l) => ctx.passed.has(l.exercise.id))) return null;
  const next = tracks.find((t) => t.prerequisite === track.slug && !t.lessons.every((l) => ctx.passed.has(l.exercise.id)));
  return {
    slug: track.slug,
    title: track.title,
    lessons: track.lessons.length,
    unaided: track.lessons.filter((l) => ctx.passed.get(l.exercise.id)?.unaided).length,
    challenges: track.lessons.filter((l) => ctx.challenges.has(l.exercise.id)).length,
    next: next ? { slug: next.slug, title: next.title } : null,
  };
}
