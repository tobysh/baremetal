import { Badge, ButtonLink, ProgressBar } from "@/components/ui";
import { lessonPath } from "@/lib/curriculum";
import type { Track } from "@/lib/curriculum/types";
import type { ProgressDoc } from "@/lib/models";

export function courseState(track: Track, progress: Map<string, ProgressDoc>) {
  const done = track.lessons.filter((lesson) => progress.get(lesson.exercise.id)?.status === "passed").length;
  const started = track.lessons.some((lesson) => progress.has(lesson.exercise.id));
  const complete = track.lessons.length > 0 && done === track.lessons.length;
  const next = track.lessons.find((lesson) => progress.get(lesson.exercise.id)?.status !== "passed") ?? track.lessons[0];
  return { done, started, complete, next, status: complete ? "Completed" : started ? "In progress" : "Not started", action: complete ? "Review course" : started ? "Continue course" : "Start course" };
}

export function CourseProgress({ track, progress = new Map(), signedIn = true }: { track: Track; progress?: Map<string, ProgressDoc>; signedIn?: boolean }) {
  const state = courseState(track, progress);
  return (
    <div className="space-y-4">
      {signedIn && <>
        <div className="flex flex-wrap items-center justify-between gap-2 text-sm">
          <Badge tone={state.complete ? "ok" : state.started ? "accent" : "neutral"}>{state.status}</Badge>
          <span className="text-muted tabular-nums">{state.done} / {track.lessons.length} lessons</span>
        </div>
        <ProgressBar value={state.done / Math.max(1, track.lessons.length)} label={`${track.title} progress`} />
      </>}
      {state.next && <ButtonLink href={lessonPath(track.slug, state.next.slug)} variant={state.started && !state.complete ? "primary" : "secondary"} className="w-full">{state.action}<span aria-hidden="true"> →</span></ButtonLink>}
    </div>
  );
}
