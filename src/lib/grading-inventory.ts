import type { Exercise, Language, Track } from "./curriculum/types";
import { gradingTrust, type GradingTrust } from "./grading-trust";

/** Server-side audit metadata only; never include test inputs or source text. */
export type GradingInventoryRow = {
  id: string;
  track: string;
  language: Language;
  kind: "lesson" | "challenge";
  trust: GradingTrust;
  sameProcessVerdict: boolean;
};

/** Uses the published curriculum loader's result, so drafts stay excluded. */
export function gradingInventory(tracks: readonly Track[]): GradingInventoryRow[] {
  const rows: GradingInventoryRow[] = [];
  const seen = new Set<string>();
  const add = (track: Track, exercise: Exercise, kind: GradingInventoryRow["kind"]) => {
    if (seen.has(exercise.id)) throw new Error(`Duplicate assessment id: ${exercise.id}`);
    seen.add(exercise.id);
    const trust = gradingTrust(exercise);
    rows.push({ id: exercise.id, track: track.slug, language: exercise.language, kind, trust, sameProcessVerdict: trust === "harness" });
  };
  for (const track of tracks) {
    for (const lesson of track.lessons) {
      add(track, lesson.exercise, "lesson");
      if (lesson.challenge) add(track, lesson.challenge.exercise, "challenge");
    }
  }
  return rows.sort((a, b) => a.id.localeCompare(b.id, "en"));
}
