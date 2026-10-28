// "Today's pick": one suggestion, chosen from what the learner has stored, with no job runner and no tracking of
// whether it was followed. The same learner sees the same pick all day.
import type { Lesson, Track } from "./curriculum/types";
import { challengeIdOf } from "./gamification";
import type { ProgressDoc, ReviewDoc } from "./models";

export type DailyPick = {
  kind: "review" | "next" | "challenge" | "revisit";
  exerciseId: string;
  track: Track;
  lesson: Lesson;
  href: string;
  label: string;
};

const lessonHref = (t: Track, l: Lesson, review = false) => `/learn/${t.slug}/${l.slug}${review ? "?review=1" : ""}`;

function seeded(key: string, n: number): number {
  let h = 2166136261;
  for (let i = 0; i < key.length; i++) h = Math.imul(h ^ key.charCodeAt(i), 16777619);
  return (h >>> 0) % n;
}

export function dailyPick(a: { tracks: Track[]; progress: Map<string, ProgressDoc>; reviews: ReviewDoc[]; userId: string; now: Date; skipExerciseId?: string }): DailyPick | null {
  const byId = new Map(a.tracks.flatMap((t) => t.lessons.map((l) => [l.exercise.id, { t, l }] as const)));
  const passed = (id: string) => a.progress.get(id)?.status === "passed";
  const out: DailyPick[] = [];

  for (const r of a.reviews) {
    const hit = byId.get(r.exerciseId);
    if (hit && r.dueAt <= a.now) out.push({ kind: "review", exerciseId: r.exerciseId, track: hit.t, lesson: hit.l, href: lessonHref(hit.t, hit.l, true), label: "Ready for a review" });
  }

  // The course the learner touched most recently comes first.
  const touched = (t: Track) => Math.max(0, ...t.lessons.map((l) => +(a.progress.get(l.exercise.id)?.updatedAt ?? 0)));
  for (const t of [...a.tracks].sort((x, y) => touched(y) - touched(x))) {
    const next = t.lessons.find((l) => !passed(l.exercise.id));
    if (next && touched(t) > 0) out.push({ kind: "next", exerciseId: next.exercise.id, track: t, lesson: next, href: lessonHref(t, next), label: "Next in your course" });
  }

  for (const t of a.tracks) {
    for (const l of t.lessons) {
      if (l.challenge && passed(l.exercise.id) && !passed(challengeIdOf(l.exercise.id))) out.push({ kind: "challenge", exerciseId: l.exercise.id, track: t, lesson: l, href: lessonHref(t, l), label: "A challenge is waiting" });
    }
  }

  const done = [...byId.entries()].filter(([id]) => passed(id)).map(([id, v]) => ({ id, ...v }));
  if (done.length) {
    const day = Math.floor(a.now.getTime() / 86_400_000);
    const pick = done[seeded(`${a.userId}:${day}`, done.length)];
    out.push({ kind: "revisit", exerciseId: pick.id, track: pick.t, lesson: pick.l, href: lessonHref(pick.t, pick.l, true), label: "Worth another look" });
  }

  return out.find((p) => p.exerciseId !== a.skipExerciseId) ?? null;
}
