import type { Metadata } from "next";
import Link from "next/link";
import { Suspense } from "react";
import { DashboardSkeleton } from "@/components/skeletons";
import { courseState } from "@/components/course-progress";
import { ButtonLink, Container, ProgressBar } from "@/components/ui";
import { UsernameNotice } from "@/components/username-notice";
import { requireUser } from "@/lib/auth";
import { getTracks, lessonPath } from "@/lib/curriculum";
import { LANGUAGE_LABEL, type Language, type Track } from "@/lib/curriculum/types";
import { levelFor } from "@/lib/gamification";
import { getProgressMap, getReviews } from "@/lib/progress";
import { ActivityStrip, CodexPreview, ContinueCard, type ContinueTarget, CourseMap, CoursePreview, PickCard, ReviewCard, RhythmCard, type DueReview } from "@/components/dashboard-sections";
import { TimezoneSync } from "@/components/timezone-sync";
import { recommendLearning } from "@/lib/learning";
import { dailyPick } from "@/lib/daily";
import { buildContext } from "@/lib/rewards-context";
import { getUnlocked } from "@/lib/rewards";
import { isChallengeId, titleBlurb } from "@/lib/gamification";

export const metadata: Metadata = {
  title: "Your dashboard",
  description: "Your progress through the Baremetal courses.",
  robots: { index: false },
};

// Short label for the language tag on a course row.
const TAG: Record<Language, string> = { c: "C", rust: "Rust", go: "Go", asm: "asm", kernel: "C" };

export default function DashboardPage() {
  return <Container width="wide" className="py-10 sm:py-12"><Suspense fallback={<DashboardSkeleton />}><DashboardBody /></Suspense></Container>;
}

type CourseEntry = { track: Track } & ReturnType<typeof courseState>;

async function DashboardBody() {
  const user = await requireUser("/dashboard");
  const [progress, reviews, unlockedRows] = await Promise.all([getProgressMap(user.id), getReviews(user.id), getUnlocked(user.id)]);
  const tracks = getTracks();
  const now = new Date();
  const ctx = buildContext(tracks, [...progress.values()], reviews, { xp: user.xp, timezone: user.timezone ?? "UTC", weeklyGoal: user.weeklyGoal }, now);
  const lessonOf = new Map(tracks.flatMap((t) => t.lessons.map((l) => [l.exercise.id, { t, l }] as const)));
  const dueReviews: DueReview[] = reviews
    .filter((r) => r.dueAt <= now && lessonOf.has(r.exerciseId))
    .map((r) => ({ href: `/learn/${lessonOf.get(r.exerciseId)!.t.slug}/${lessonOf.get(r.exerciseId)!.l.slug}?review=1`, title: lessonOf.get(r.exerciseId)!.l.title, trackTitle: lessonOf.get(r.exerciseId)!.t.title }));
  const unlocked = new Map(unlockedRows.map((r) => [r.id, r.unlockedAt]));
  const allLessons = tracks.flatMap((track) => track.lessons.map((lesson) => ({ track, lesson })));
  const passedCount = allLessons.filter(({ lesson }) => progress.get(lesson.exercise.id)?.status === "passed").length;
  const courses: CourseEntry[] = tracks.map((track) => ({ track, ...courseState(track, progress) }));
  const completedCount = courses.filter((course) => course.complete).length;
  const bySlug = new Map(courses.map((course) => [course.track.slug, course]));
  const prerequisite = (course: CourseEntry) => (course.track.prerequisite ? bySlug.get(course.track.prerequisite) : undefined);

  const nextUp = recommendLearning(tracks, progress);
  const nextUpDone = nextUp ? courseState(nextUp.track, progress).done : 0;

  // Other courses already under way, so a second course in progress is still reachable from here.
  const alsoActive = courses.filter((course) => course.started && !course.complete && course.track.slug !== nextUp?.track.slug);
  // Courses whose prerequisite is complete come first (the sort is stable, so catalogue order holds otherwise).
  const ready = (course: CourseEntry) => Number(!prerequisite(course) || prerequisite(course)!.complete);
  const notStarted = courses.filter((course) => !course.started).sort((a, b) => ready(b) - ready(a)).slice(0, 4);

  const pick = dailyPick({ tracks, progress, reviews, userId: user.id, now, skipExerciseId: nextUp?.lesson.exercise.id });
  const lastTouched = [...progress.values()]
    .filter((p) => p.status !== "passed" && !isChallengeId(p.exerciseId) && lessonOf.has(p.exerciseId) && p.exerciseId !== nextUp?.lesson.exercise.id)
    .sort((a, b) => +b.updatedAt - +a.updatedAt)[0];
  const continueTarget: ContinueTarget | null = lastTouched
    ? { href: lessonPath(lessonOf.get(lastTouched.exerciseId)!.t.slug, lessonOf.get(lastTouched.exerciseId)!.l.slug), title: lessonOf.get(lastTouched.exerciseId)!.l.title, trackTitle: lessonOf.get(lastTouched.exerciseId)!.t.title, updatedAt: lastTouched.updatedAt }
    : null;
  const level = levelFor(user.xp);
  const heroLabel = progress.size === 0 ? "Start first lesson" : nextUpDone === 0 && progress.get(nextUp?.lesson.exercise.id ?? "")?.status !== "started" ? "Start lesson" : "Continue";
  const xpFill = level.next === null ? 1 : user.xp / level.next;

  const previewCourse = nextUp ? { track: nextUp.track, done: nextUpDone, nextTitle: nextUp.lesson.title } : null;

  return (
    <div className="flex flex-col gap-4 sm:gap-6">
      {!user.timezone && <TimezoneSync />}

      {/* Row 1: the next action plus a compact level summary. Rows size to their content, nothing is stretched to match a neighbour. */}
      <div className="grid items-start gap-4 sm:gap-6 lg:grid-cols-[minmax(0,1fr)_minmax(300px,360px)]">
        {nextUp ? (
          <section aria-label="Resume course" className="panel flex min-w-0 flex-col gap-5 p-5 sm:p-8">
            <div className="flex flex-col">
              <p className="text-sm text-muted">
                {nextUp.track.title} <span className="text-line-strong">/</span> Lesson {nextUp.track.lessons.indexOf(nextUp.lesson) + 1} of {nextUp.track.lessons.length} <span className="text-line-strong">/</span> {LANGUAGE_LABEL[nextUp.track.language]}, {nextUp.track.level}
              </p>
              <h1 className="display mt-3 text-2xl font-bold leading-[1.1] sm:text-[34px]">{nextUp.lesson.title}</h1>
              <p className="mt-2 line-clamp-2 max-w-[560px] text-base leading-relaxed text-text-2 sm:line-clamp-none">{nextUp.lesson.summary}</p>
            </div>
            <div className="flex flex-wrap items-center gap-x-10 gap-y-4">
              <div className="flex min-w-0 flex-[1_1_240px] flex-col gap-2">
                <p className="text-sm text-text-2"><span className="font-semibold text-text">{nextUpDone}</span> of {nextUp.track.lessons.length} lessons completed</p>
                <ProgressBar value={nextUpDone / nextUp.track.lessons.length} label={`${nextUp.track.title} progress`} />
              </div>
              <ButtonLink href={lessonPath(nextUp.track.slug, nextUp.lesson.slug)} size="lg" className="w-full flex-none sm:w-auto">
                {heroLabel} <span className="text-sm font-normal">{nextUp.lesson.minutes} min</span>
              </ButtonLink>
            </div>
          </section>
        ) : (
          <section aria-label="All lessons passed" className="panel flex min-w-0 flex-col gap-5 p-5 sm:p-8">
            <div>
              <h1 className="display text-2xl font-bold leading-[1.1] sm:text-[34px]">Every lesson passed.</h1>
              <p className="mt-2 max-w-[560px] text-base leading-relaxed text-text-2">Revisit a course, try a different solution, or build something of your own.</p>
            </div>
            <div><ButtonLink href="/learn" size="lg">Review courses</ButtonLink></div>
          </section>
        )}

        <aside aria-label="Your progress" className="panel flex min-w-0 flex-col p-4 sm:p-6">
          <div className="flex items-baseline justify-between gap-3">
            <span className="display text-xl font-bold leading-tight sm:text-2xl">{level.title}</span>
            <span className="text-sm font-semibold text-accent">Level {level.level}</span>
          </div>
          <span className="mt-1 hidden text-sm text-muted lg:block">{titleBlurb(level.level)}</span>
          <div role="img" aria-label={level.next === null ? `${user.xp} XP, top level` : `${user.xp} of ${level.next} XP`} className="mt-3 h-2 overflow-hidden rounded-sm bg-line">
            <div className="h-full bg-accent" style={{ width: `${Math.round(Math.min(1, xpFill) * 100)}%` }} />
          </div>
          <div className="mt-2 flex flex-wrap justify-between gap-x-2 text-sm">
            <span className="font-medium">{user.xp}{level.next !== null && ` / ${level.next}`} XP</span>
            <span className="text-muted">{level.next === null ? "Top level. Challenges still earn XP." : `${level.next - user.xp} XP to level ${level.level + 1}`}</span>
          </div>
          <p className="mt-3 hidden text-sm text-muted sm:block">
            {passedCount} of {allLessons.length} lessons passed · {completedCount} of {tracks.length} courses completed
          </p>
        </aside>
      </div>

      {/* Row 2: review, resume and weekly rhythm as segments of one panel. */}
      <ActivityStrip>
        <ReviewCard due={dueReviews} />
        {continueTarget && <ContinueCard target={continueTarget} />}
        {pick && !(pick.kind === "review" && dueReviews.length > 0) && <PickCard pick={pick} />}
        <RhythmCard rhythm={ctx.rhythm} />
      </ActivityStrip>

      {/* Phones get a one-course preview; from md up the collapsible map (active course open, other paths closed). */}
      {previewCourse && <CoursePreview {...previewCourse} />}
      <CourseMap tracks={tracks} progress={progress} recommendedSlug={nextUp?.track.slug} upNextLessonSlug={nextUp?.lesson.slug} />

      {alsoActive.length > 0 && (
        <CoursePanel id="also-active" title="In progress" count={alsoActive.length}>
          {alsoActive.map(({ track, done, next }) => (
            <CourseLink key={track.slug} track={track} href={lessonPath(track.slug, next.slug)} note={`${done} of ${track.lessons.length} lessons complete`} action="Continue" />
          ))}
        </CoursePanel>
      )}

      {notStarted.length > 0 && (
        <div className="hidden md:block">
          <CoursePanel id="not-started" title="Not started" count={notStarted.length} more>
            {notStarted.map((course, i) => {
              const pre = prerequisite(course);
              const builds = pre ? `Builds on ${pre.track.title}` : undefined;
              const recommended = i === 0 && ready(course) === 1;
              return (
                <CourseLink
                  key={course.track.slug}
                  track={course.track}
                  href={`/learn/${course.track.slug}`}
                  note={[recommended ? "Recommended next" : undefined, builds ?? (recommended ? undefined : "No prerequisites")].filter(Boolean).join(" · ")}
                  recommended={recommended}
                  action="View"
                />
              );
            })}
          </CoursePanel>
        </div>
      )}

      {/* Detail lives on its own pages: Codex and numbers. */}
      <div className="grid items-start gap-4 sm:gap-6 md:grid-cols-2">
        <CodexPreview unlocked={unlocked} />
        <section aria-labelledby="numbers-h" className="panel flex min-w-0 flex-col p-4 sm:p-5">
          <div className="flex items-baseline justify-between gap-3">
            <h2 id="numbers-h" className="display-sm text-base font-bold">Your numbers</h2>
            <Link href="/dashboard/stats" className="inline-flex min-h-9 items-center text-sm font-medium text-accent hover:underline">View stats</Link>
          </div>
          <p className="mt-2 text-sm text-muted">XP by month, seals, hints and progress by course. Only you can see these.</p>
        </section>
      </div>

      {!user.username && <UsernameNotice userId={user.id} />}
    </div>
  );
}

function CoursePanel({ id, title, count, more, children }: { id: string; title: string; count: number; more?: boolean; children: React.ReactNode }) {
  return (
    <section aria-labelledby={id} className="panel flex flex-col p-3">
      <div className="flex items-baseline justify-between gap-3 px-5 pb-4 pt-5">
        <h2 id={id} className="display text-[21px] font-bold">{title} <span className="text-accent">{count}</span></h2>
        {more && <Link href="/learn" className="text-sm font-medium text-muted hover:text-accent">All courses</Link>}
      </div>
      <div className="flex flex-col gap-0.5">{children}</div>
    </section>
  );
}

function CourseLink({ track, href, note, recommended, action }: { track: Track; href: string; note: string; recommended?: boolean; action: string }) {
  const minutes = track.lessons.reduce((sum, lesson) => sum + lesson.minutes, 0);
  // Phones: badge beside the title, then one meta line under it (the whole row is the link, so the button is hidden). From sm up the parts
  // become the columns of a single row (the meta wrapper uses display: contents).
  return (
    <Link href={href} className="group grid grid-cols-[auto_minmax(0,1fr)_auto] items-center gap-x-4 gap-y-1 rounded-[10px] px-4 py-4 hover:bg-panel-2 sm:flex sm:flex-wrap sm:gap-x-5 sm:gap-y-3 sm:px-5">
      <span
        className={`flex h-11 w-12 flex-none items-center justify-center rounded-md border font-mono text-[13px] font-medium group-hover:border-accent ${recommended ? "border-accent bg-accent text-accent-ink" : "border-line-strong"}`}
      >
        {TAG[track.language]}
      </span>
      <span className="col-span-2 flex min-w-0 flex-col gap-0.5 sm:flex-[1_1_260px]">
        <span className="display-sm text-[17px] font-semibold">{track.title}</span>
        <span className={`text-sm ${recommended ? "text-accent" : "text-muted"}`}>{note}</span>
      </span>
      <span className="col-span-2 col-start-2 flex items-center gap-x-2 whitespace-nowrap text-sm text-text-2 sm:contents">
        <span className="sm:flex-[0_0_104px]">{track.level}</span>
        <span aria-hidden="true" className="text-line-strong sm:hidden">·</span>
        <span className="sm:flex-[0_0_88px]">{track.lessons.length} lessons</span>
        <span aria-hidden="true" className="text-line-strong sm:hidden">·</span>
        <span className="sm:flex-[0_0_40px]">{Math.max(1, Math.round(minutes / 60))} h</span>
      </span>
      <span className="hidden min-h-9 items-center justify-center rounded-md border border-line-strong px-4 text-sm font-medium text-text-2 group-hover:border-accent group-hover:text-accent sm:inline-flex">{action}</span>
    </Link>
  );
}
