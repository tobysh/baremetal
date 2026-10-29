import "server-only";
import { io } from "next/cache";
import { getDb } from "./db";
import { getTracks } from "./curriculum";

/** Lessons with fewer started learners than this are left out of the rankings: too noisy, and too close to one person. */
export const MIN_LEARNERS = 5;

export type StuckRow = {
  id: string;
  title: string;
  track: string;
  href: string;
  challenge: boolean;
  started: number;
  passed: number;
  /** Share of learners who started and have passed, 0 to 1. */
  passRate: number;
  /** Mean failed-or-total runs before the first pass (attemptsAtPass), over learners with a recorded snapshot. */
  avgAttemptsToPass: number | null;
  /** Mean hints opened per learner who started. */
  avgHints: number;
  /** Share of starters who opened at least one hint. */
  hintShare: number;
  tutorQuestions: number;
  tutorLearners: number;
};

export type StuckReport = { rows: StuckRow[]; totalLessons: number; hidden: number; minLearners: number };

/**
 * Aggregates per lesson across all learners. Only counts leave this module: no user ids, names or emails.
 * Sources: `progress` (status, attempts snapshots, hints), `tutor_usage` (questions) and `tutor_threads` (distinct learners).
 */
export async function getStuckReport(): Promise<StuckReport> {
  await io();
  const db = await getDb();
  const [progress, usage, threads] = await Promise.all([
    db
      .collection("progress")
      .aggregate([
        {
          $group: {
            _id: "$exerciseId",
            started: { $sum: 1 },
            passed: { $sum: { $cond: [{ $eq: ["$status", "passed"] }, 1, 0] } },
            attemptsSum: { $sum: { $cond: [{ $and: [{ $eq: ["$status", "passed"] }, { $isNumber: "$attemptsAtPass" }] }, "$attemptsAtPass", 0] } },
            attemptsN: { $sum: { $cond: [{ $and: [{ $eq: ["$status", "passed"] }, { $isNumber: "$attemptsAtPass" }] }, 1, 0] } },
            hintsSum: { $sum: { $ifNull: ["$hintsUsed", 0] } },
            hintLearners: { $sum: { $cond: [{ $gt: [{ $ifNull: ["$hintsUsed", 0] }, 0] }, 1, 0] } },
          },
        },
      ])
      .toArray(),
    db.collection("tutor_usage").aggregate([{ $group: { _id: "$exerciseId", questions: { $sum: 1 } } }]).toArray(),
    db.collection("tutor_threads").aggregate([{ $group: { _id: "$lessonId", learners: { $sum: 1 } } }]).toArray(),
  ]);

  const names = new Map<string, { title: string; track: string; href: string; challenge: boolean }>();
  let totalLessons = 0;
  for (const t of getTracks()) {
    for (const l of t.lessons) {
      totalLessons++;
      const href = `/learn/${t.slug}/${l.slug}`;
      names.set(l.exercise.id, { title: l.title, track: t.title, href, challenge: false });
      if (l.challenge) names.set(l.challenge.exercise.id, { title: `${l.title} (challenge)`, track: t.title, href, challenge: true });
    }
  }
  const q = new Map(usage.map((u) => [String(u._id), u.questions as number]));
  const tl = new Map(threads.map((u) => [String(u._id), u.learners as number]));

  const all: StuckRow[] = [];
  let hidden = 0;
  for (const p of progress) {
    const id = String(p._id);
    const meta = names.get(id);
    if (!meta) continue; // lesson since removed
    if (p.started < MIN_LEARNERS) {
      hidden++;
      continue;
    }
    all.push({
      id,
      ...meta,
      started: p.started,
      passed: p.passed,
      passRate: p.passed / p.started,
      avgAttemptsToPass: p.attemptsN > 0 ? p.attemptsSum / p.attemptsN : null,
      avgHints: p.hintsSum / p.started,
      hintShare: p.hintLearners / p.started,
      tutorQuestions: q.get(id) ?? 0,
      tutorLearners: tl.get(id) ?? 0,
    });
  }
  return { rows: all, totalLessons, hidden, minLearners: MIN_LEARNERS };
}

export const top = <T,>(rows: T[], by: (r: T) => number, n = 10) => [...rows].sort((a, b) => by(b) - by(a)).slice(0, n);
