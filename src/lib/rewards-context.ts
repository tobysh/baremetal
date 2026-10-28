// Builds the Codex context from stored documents. No server-only import, so scripts can use it too.
import type { CodexContext } from "./achievements";
import type { Track } from "./curriculum/types";
import { challengeIdOf, isChallengeId } from "./gamification";
import type { ProgressDoc, ReviewDoc, UserDoc } from "./models";
import { computeRhythm } from "./rhythm";

export type Basics = Pick<UserDoc, "xp" | "timezone" | "weeklyGoal">;

export function buildContext(tracks: Track[], progress: ProgressDoc[], reviews: ReviewDoc[], user: Basics, now = new Date()): CodexContext {
  const passed = new Map<string, { passedAt: Date; attemptsAtPass: number; unaided: boolean }>();
  const challenges = new Set<string>();
  const events: Date[] = [];
  for (const p of progress) {
    if (p.status !== "passed") continue;
    if (!p.xpWithheld && p.passedAt) events.push(p.passedAt);
    if (isChallengeId(p.exerciseId)) challenges.add(p.exerciseId.slice(0, p.exerciseId.length - "#challenge".length));
    else passed.set(p.exerciseId, { passedAt: p.passedAt ?? new Date(0), attemptsAtPass: p.attemptsAtPass ?? p.attempts, unaided: !!p.unaided });
  }
  return {
    tracks: tracks.map((t) => ({ slug: t.slug, language: t.language, lessons: t.lessons.map((l) => ({ id: l.exercise.id, difficulty: l.difficulty, challenge: !!l.challenge })) })),
    passed,
    challenges,
    xp: user.xp ?? 0,
    rhythm: computeRhythm(events, user.weeklyGoal, user.timezone ?? "UTC", now),
    reviewsPassed: reviews.reduce((n, r) => n + (r.count ?? 0), 0),
    reviewed: new Set(reviews.filter((r) => (r.count ?? 0) > 0).map((r) => r.exerciseId)),
  };
}

export type Seals = { passed: boolean; unaided: boolean; challenge: boolean };

/** The three seals a lesson can show. `hasChallenge` tells the UI whether the third one exists for this lesson. */
export function sealsFor(progress: Map<string, ProgressDoc>, lessonExerciseId: string): Seals {
  const p = progress.get(lessonExerciseId);
  return { passed: p?.status === "passed", unaided: !!p?.unaided, challenge: progress.get(challengeIdOf(lessonExerciseId))?.status === "passed" };
}
