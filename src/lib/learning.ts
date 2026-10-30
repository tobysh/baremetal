import type { Lesson, Track } from "./curriculum/types";

export type LearningProgress = { exerciseId: string; status: string; updatedAt: Date };
export type Recommendation = { track: Track; lesson: Lesson };

export function unfinishedLesson(track: Track, progress: ReadonlyMap<string, LearningProgress>) {
  return track.lessons.find((lesson) => progress.get(lesson.exercise.id)?.status !== "passed");
}

export function courseComplete(track: Track, progress: ReadonlyMap<string, LearningProgress>) {
  return track.lessons.length > 0 && !unfinishedLesson(track, progress);
}

/** Prerequisites are advice, never access restrictions. Catalogue order breaks ties. */
export function nextCourse(tracks: Track[], current: Track, progress: ReadonlyMap<string, LearningProgress>) {
  const path = current.slug === "first-steps" ? "c" : current.path;
  const bySlug = new Map(tracks.map((track) => [track.slug, track]));
  return tracks.find((track) => {
    if (track.slug === current.slug || track.path !== path || courseComplete(track, progress) || !track.lessons.length) return false;
    if (current.path === path && tracks.indexOf(track) <= tracks.indexOf(current)) return false;
    if (!track.prerequisite || track.prerequisite === current.slug) return true;
    const prerequisite = bySlug.get(track.prerequisite);
    return !!prerequisite && courseComplete(prerequisite, progress);
  });
}

/** An explicit course context is used by catalogue actions and completion banners. */
export function recommendLearning(tracks: Track[], progress: ReadonlyMap<string, LearningProgress>, context?: { track: Track; afterPass?: string }): Recommendation | null {
  let effective = progress;
  if (context?.afterPass) {
    effective = new Map(progress);
    (effective as Map<string, LearningProgress>).set(context.afterPass, { exerciseId: context.afterPass, status: "passed", updatedAt: new Date(0) });
  }
  const lessons = tracks.flatMap((track) => track.lessons.map((lesson) => ({ track, lesson })));
  const recent = lessons.filter(({ lesson }) => effective.has(lesson.exercise.id))
    .sort((a, b) => +effective.get(b.lesson.exercise.id)!.updatedAt - +effective.get(a.lesson.exercise.id)!.updatedAt)[0];
  const current = context?.track ?? recent?.track ?? tracks.find((track) => track.slug === "first-steps") ?? tracks[0];
  if (!current) return null;
  if (!context && recent && effective.get(recent.lesson.exercise.id)?.status !== "passed") return recent;
  const lesson = unfinishedLesson(current, effective);
  if (lesson) return { track: current, lesson };
  const next = nextCourse(tracks, current, effective);
  const nextLesson = next && unfinishedLesson(next, effective);
  return next && nextLesson ? { track: next, lesson: nextLesson } : null;
}

export function validateLearningPaths(tracks: Track[]) {
  const bySlug = new Map(tracks.map((track) => [track.slug, track]));
  for (const track of tracks) {
    if (!Array.isArray(track.outcomes) || !track.outcomes.length || track.outcomes.some((value) => typeof value !== "string" || !value.trim())) throw new Error(`${track.slug}: needs authored course outcomes`);
    const seen = new Set<string>();
    let current: Track | undefined = track;
    while (current) {
      if (seen.has(current.slug)) throw new Error(`${track.slug}: prerequisite cycle at ${current.slug}`);
      seen.add(current.slug);
      if (current.prerequisite && !bySlug.has(current.prerequisite)) throw new Error(`${current.slug}: missing prerequisite ${current.prerequisite}`);
      current = current.prerequisite ? bySlug.get(current.prerequisite) : undefined;
    }
  }
}
