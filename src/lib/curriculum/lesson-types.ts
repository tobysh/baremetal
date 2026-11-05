// Lesson "types" for the catalogue filter (`?type=`): code lessons, reviews, challenges and the practice kinds.
import type { PracticeKind } from "./practice";
import type { Lesson, Track } from "./types";

export const TYPE_FILTERS = ["lesson", "review", "challenge", "parsons", "blanks", "predict", "bug", "choice", "typed", "match"] as const;
export type LessonType = (typeof TYPE_FILTERS)[number];

export const TYPE_LABEL: Record<LessonType, string> = {
  lesson: "Code lesson",
  review: "Review",
  challenge: "Challenge",
  parsons: "Arrange lines",
  blanks: "Fill blanks",
  predict: "Predict output",
  bug: "Find the bug",
  choice: "Multiple choice",
  typed: "Type the answer",
  match: "Match pairs",
};

export const isLessonType = (v: unknown): v is LessonType => typeof v === "string" && (TYPE_FILTERS as readonly string[]).includes(v);

/** Validates a raw `?type=` search param (a single value); anything else means no filter. */
export const parseTypeFilter = (v: string | string[] | undefined): LessonType | null => (typeof v === "string" && isLessonType(v) ? v : null);

/** A lesson's types: `review` for review lessons, otherwise `lesson`; plus `challenge` and its practice kind when present. */
export function lessonTypes(lesson: Pick<Lesson, "kind" | "challenge" | "practice">): LessonType[] {
  const out: LessonType[] = [lesson.kind === "review" ? "review" : "lesson"];
  if (lesson.challenge) out.push("challenge");
  if (lesson.practice) out.push(lesson.practice.kind satisfies PracticeKind);
  return out;
}

export type TypeCounts = Record<LessonType, number>;

/** Number of lessons of each type in a track, so the UI can hide filters with nothing behind them. */
export function trackTypeCounts(track: Pick<Track, "lessons">): TypeCounts {
  const counts = Object.fromEntries(TYPE_FILTERS.map((t) => [t, 0])) as TypeCounts;
  for (const l of track.lessons) for (const t of lessonTypes(l)) counts[t]++;
  return counts;
}

/** Counts across several tracks (per-lesson, summed). */
export function typeCounts(tracks: Pick<Track, "lessons">[]): TypeCounts {
  const total = Object.fromEntries(TYPE_FILTERS.map((t) => [t, 0])) as TypeCounts;
  for (const t of tracks) for (const [k, v] of Object.entries(trackTypeCounts(t))) total[k as LessonType] += v;
  return total;
}
