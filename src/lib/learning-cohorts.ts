import type { Track } from "./curriculum/types";

const DAY = 86_400_000;
export const COHORT_MIN_LEARNERS = 5;
export type CohortProgress = { exerciseId: string; startedAt?: Date; passedAt?: Date; status: string };
export type LearnerCourse = { trackSlug: string; lessons: CohortProgress[] };
export type LearningCohort = { trackSlug: string; title: string; week: string; learners: number; firstLessonPassed: number; completed: number; medianFirstPassHours: number | null; laterActivity: number; mature: boolean };

const valid = (date: Date | undefined): date is Date => date instanceof Date && Number.isFinite(+date);
const weekOf = (date: Date) => {
  const monday = new Date(Date.UTC(date.getUTCFullYear(), date.getUTCMonth(), date.getUTCDate()));
  monday.setUTCDate(monday.getUTCDate() - (monday.getUTCDay() + 6) % 7);
  return monday.toISOString().slice(0, 10);
};

/** Input is grouped inside MongoDB; no identifiers leave this function. */
export function buildLearningCohorts(tracks: Track[], groups: LearnerCourse[], now = new Date()) {
  const bySlug = new Map(tracks.map((track) => [track.slug, track]));
  const buckets = new Map<string, { row: LearningCohort; durations: number[] }>();
  let excluded = 0;
  for (const group of groups) {
    const track = bySlug.get(group.trackSlug);
    if (!track?.lessons.length) continue;
    const ids = new Set(track.lessons.map((lesson) => lesson.exercise.id));
    const lessons = group.lessons.filter((lesson) => ids.has(lesson.exerciseId));
    // A partial legacy record could make a known timestamp look like the first start.
    if (!lessons.length || lessons.some((lesson) => !valid(lesson.startedAt) || +lesson.startedAt > +now)) { excluded++; continue; }
    const start = Math.min(...lessons.map((lesson) => +lesson.startedAt!));
    const week = weekOf(new Date(start));
    const key = `${track.slug}:${week}`;
    let bucket = buckets.get(key);
    if (!bucket) {
      bucket = { row: { trackSlug: track.slug, title: track.title, week, learners: 0, firstLessonPassed: 0, completed: 0, medianFirstPassHours: null, laterActivity: 0, mature: +now >= +new Date(week) + 21 * DAY }, durations: [] };
      buckets.set(key, bucket);
    }
    const row = bucket.row;
    row.learners++;
    const passed = lessons.filter((lesson) => lesson.status === "passed");
    if (passed.some((lesson) => lesson.exerciseId === track.lessons[0].exercise.id)) row.firstLessonPassed++;
    if (new Set(passed.map((lesson) => lesson.exerciseId)).size === ids.size) row.completed++;
    const passDates = passed.filter((lesson) => valid(lesson.passedAt) && +lesson.passedAt >= start && +lesson.passedAt <= +now).map((lesson) => +lesson.passedAt!);
    if (passDates.length) bucket.durations.push((Math.min(...passDates) - start) / 3_600_000);
    const activity = lessons.flatMap((lesson) => [lesson.startedAt, ...(lesson.status === "passed" ? [lesson.passedAt] : [])]);
    if (activity.some((date) => valid(date) && +date >= start + 7 * DAY && +date < start + 14 * DAY && +date <= +now)) row.laterActivity++;
  }
  let hidden = 0;
  const rows: LearningCohort[] = [];
  for (const { row, durations } of buckets.values()) {
    if (row.learners < COHORT_MIN_LEARNERS) { hidden++; continue; }
    durations.sort((a, b) => a - b);
    const middle = Math.floor(durations.length / 2);
    row.medianFirstPassHours = durations.length ? (durations[middle] + durations[Math.floor((durations.length - 1) / 2)]) / 2 : null;
    rows.push(row);
  }
  rows.sort((a, b) => b.week.localeCompare(a.week) || a.title.localeCompare(b.title));
  return { rows, hidden, excluded, minLearners: COHORT_MIN_LEARNERS };
}
