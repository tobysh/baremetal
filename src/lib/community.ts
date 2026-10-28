import "server-only";
import { getDb } from "./db";
import { getTracks } from "./curriculum";
import type { Difficulty } from "./gamification";
import { achievementById } from "./achievements";
import type { AchievementDoc, ProgressDoc, ReviewDoc, UserDoc } from "./models";
import { buildContext } from "./rewards-context";

// Only what a learner chose to make public: username, XP and completed
// lessons. Never names, emails or code.

const PUBLIC = { profilePublic: { $ne: false }, username: { $type: "string" } } as const;
type PublicUser = Pick<UserDoc, "_id" | "username" | "xp" | "createdAt" | "timezone" | "weeklyGoal">;
const projection = { username: 1, xp: 1, createdAt: 1, timezone: 1, weeklyGoal: 1 } as const;

export type LeaderboardRow = { rank: number; username: string; xp: number };

export async function getLeaderboard(limit = 50): Promise<LeaderboardRow[]> {
  const db = await getDb();
  const rows = await db
    .collection<UserDoc>("users")
    .find(PUBLIC, { projection })
    .sort({ xp: -1, createdAt: 1 })
    .limit(limit)
    .toArray();
  return rows.map((u, i) => ({ rank: i + 1, username: u.username!, xp: u.xp ?? 0 }));
}

/** Prefix search on usernames (which are restricted to [a-z0-9_-], so safe in a regex). */
export async function searchUsers(query: string, limit = 20): Promise<{ username: string; xp: number }[]> {
  const q = query.trim().toLowerCase().replace(/[^a-z0-9_-]/g, "").slice(0, 20);
  if (!q) return [];
  const db = await getDb();
  const rows = await db
    .collection<UserDoc>("users")
    .find({ ...PUBLIC, username: { $regex: `^${q}` } }, { projection })
    .sort({ xp: -1 })
    .limit(limit)
    .toArray();
  return rows.map((u) => ({ username: u.username!, xp: u.xp ?? 0 }));
}

export type PublicProfile = {
  username: string;
  xp: number;
  rank: number;
  memberSince: Date;
  completed: number;
  totalLessons: number;
  tracks: { slug: string; title: string; done: number; total: number; unaided: number; challenges: number }[];
  achievements: { name: string; description: string; unlockedAt: Date }[];
  /** Longest run of weeks the weekly goal was met. The current week is never shown. */
  bestRhythm: number;
  recent: { title: string; trackTitle: string; difficulty: Difficulty; passedAt: Date }[];
};

export async function getPublicProfile(username: string): Promise<PublicProfile | null> {
  const db = await getDb();
  const user = (await db.collection<UserDoc>("users").findOne({ ...PUBLIC, username: username.toLowerCase() }, { projection })) as PublicUser | null;
  if (!user) return null;
  const xp = user.xp ?? 0;
  const [ahead, passed, allProgress, reviews, earned] = await Promise.all([
    db.collection<UserDoc>("users").countDocuments({ ...PUBLIC, xp: { $gt: xp } }),
    db
      .collection<ProgressDoc>("progress")
      .find({ userId: user._id, status: "passed" }, { projection: { exerciseId: 1, passedAt: 1 } })
      .sort({ passedAt: -1 })
      .toArray(),
    db.collection<ProgressDoc>("progress").find({ userId: user._id, status: "passed" }, { projection: { code: 0 } }).toArray(),
    db.collection<ReviewDoc>("reviews").find({ userId: user._id }).toArray(),
    db.collection<AchievementDoc>("achievements").find({ userId: user._id }).sort({ unlockedAt: -1 }).toArray(),
  ]);
  const ctx = buildContext(getTracks(), allProgress, reviews, user);
  const passedIds = new Set(passed.map((p) => p.exerciseId));
  const tracks = getTracks();
  const lessonById = new Map(tracks.flatMap((t) => t.lessons.map((l) => [l.exercise.id, { lesson: l, track: t }] as const)));
  return {
    username: user.username!,
    xp,
    rank: ahead + 1,
    memberSince: user.createdAt,
    completed: passed.filter((p) => lessonById.has(p.exerciseId)).length,
    totalLessons: lessonById.size,
    tracks: tracks.map((t) => ({
      slug: t.slug,
      title: t.title,
      done: t.lessons.filter((l) => passedIds.has(l.exercise.id)).length,
      total: t.lessons.length,
      unaided: t.lessons.filter((l) => ctx.passed.get(l.exercise.id)?.unaided).length,
      challenges: t.lessons.filter((l) => ctx.challenges.has(l.exercise.id)).length,
    })),
    achievements: earned.flatMap((a) => {
      const def = achievementById.get(a.id);
      return def ? [{ name: def.name, description: def.description, unlockedAt: a.unlockedAt }] : [];
    }),
    bestRhythm: ctx.rhythm.best,
    recent: passed
      .filter((p) => lessonById.has(p.exerciseId))
      .slice(0, 8)
      .map((p) => {
        const { lesson, track } = lessonById.get(p.exerciseId)!;
        return { title: lesson.title, trackTitle: track.title, difficulty: lesson.difficulty, passedAt: p.passedAt ?? new Date(0) };
      }),
  };
}
