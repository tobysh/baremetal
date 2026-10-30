import type { Metadata } from "next";
import Link from "next/link";
import { Suspense } from "react";
import { DashboardSkeleton } from "@/components/skeletons";
import { courseState } from "@/components/course-progress";
import { ButtonLink, Container } from "@/components/ui";
import { UsernameNotice } from "@/components/username-notice";
import { requireUser } from "@/lib/auth";
import { getTracks, lessonPath } from "@/lib/curriculum";
import { LANGUAGE_LABEL, type Language, type Track } from "@/lib/curriculum/types";
import { levelFor } from "@/lib/gamification";
import type { ProgressDoc } from "@/lib/models";
import { getProgressMap, getReviews } from "@/lib/progress";
import { CodexPanel, CourseMap, PickCard, ReviewCard, RhythmCard, StatsPanel, type DueReview, type MonthXp } from "@/components/dashboard-sections";
import { TimezoneSync } from "@/components/timezone-sync";
import { recommendLearning } from "@/lib/learning";
import { dailyPick } from "@/lib/daily";
import { getDb } from "@/lib/db";
import { ObjectId } from "mongodb";
import { buildContext } from "@/lib/rewards-context";
import { getUnlocked } from "@/lib/rewards";
import { isChallengeId, titleBlurb } from "@/lib/gamification";
import type { XpEventDoc } from "@/lib/models";

export const metadata: Metadata = {
  title: "Your dashboard",
  description: "Your progress through the Baremetal courses.",
  robots: { index: false },
};

// Short label for the language tag on a course row.
const TAG: Record<Language, string> = { c: "C", rust: "Rust", go: "Go", asm: "asm", kernel: "C" };

export default function DashboardPage() {
  return <Container className="py-10 sm:py-12"><Suspense fallback={<DashboardSkeleton />}><DashboardBody /></Suspense></Container>;
}

type CourseEntry = { track: Track } & ReturnType<typeof courseState>;

async function DashboardBody() {
  const user = await requireUser("/dashboard");
  const [progress, reviews, unlockedRows, xpEvents] = await Promise.all([
    getProgressMap(user.id),
    getReviews(user.id),
    getUnlocked(user.id),
    (await getDb()).collection<XpEventDoc>("xp_events").find({ userId: new ObjectId(user.id) }, { projection: { xp: 1, at: 1 } }).toArray(),
  ]);
  const tracks = getTracks();
  const now = new Date();
  const ctx = buildContext(tracks, [...progress.values()], reviews, { xp: user.xp, timezone: user.timezone ?? "UTC", weeklyGoal: user.weeklyGoal }, now);
  const lessonOf = new Map(tracks.flatMap((t) => t.lessons.map((l) => [l.exercise.id, { t, l }] as const)));
  const dueReviews: DueReview[] = reviews
    .filter((r) => r.dueAt <= now && lessonOf.has(r.exerciseId))
    .map((r) => ({ href: `/learn/${lessonOf.get(r.exerciseId)!.t.slug}/${lessonOf.get(r.exerciseId)!.l.slug}?review=1`, title: lessonOf.get(r.exerciseId)!.l.title, trackTitle: lessonOf.get(r.exerciseId)!.t.title }));
  const unlocked = new Map(unlockedRows.map((r) => [r.id, r.unlockedAt]));
  const months: MonthXp[] = Array.from({ length: 6 }, (_, i) => {
    const d = new Date(Date.UTC(now.getUTCFullYear(), now.getUTCMonth() - (5 - i), 1));
    const next = new Date(Date.UTC(d.getUTCFullYear(), d.getUTCMonth() + 1, 1));
    return { label: d.toLocaleDateString("en-GB", { month: "short", timeZone: "UTC" }), xp: xpEvents.filter((e) => e.at >= d && e.at < next).reduce((n, e) => n + e.xp, 0) };
  });
  const passedDocs = [...progress.values()].filter((p) => p.status === "passed" && !isChallengeId(p.exerciseId)).sort((a, b) => +(b.passedAt ?? 0) - +(a.passedAt ?? 0));
  const avgHints = (docs: typeof passedDocs) => docs.reduce((n, p) => n + (p.hintsUsedAtPass ?? 0), 0) / Math.max(1, docs.length);
  const recentDocs = passedDocs.slice(0, 10);
  const earlierDocs = passedDocs.slice(10, 20);
  const allLessons = tracks.flatMap((track) => track.lessons.map((lesson) => ({ track, lesson })));
  const passedCount = allLessons.filter(({ lesson }) => progress.get(lesson.exercise.id)?.status === "passed").length;
  const courses: CourseEntry[] = tracks.map((track) => ({ track, ...courseState(track, progress) }));
  const completedCount = courses.filter((course) => course.complete).length;
  const bySlug = new Map(courses.map((course) => [course.track.slug, course]));
  const prerequisite = (course: CourseEntry) => (course.track.prerequisite ? bySlug.get(course.track.prerequisite) : undefined);

  const nextUp = recommendLearning(tracks, progress);

  // Other courses already under way, so a second course in progress is still reachable from here.
  const alsoActive = courses.filter((course) => course.started && !course.complete && course.track.slug !== nextUp?.track.slug);
  // Courses whose prerequisite is complete come first (the sort is stable, so catalogue order holds otherwise).
  const ready = (course: CourseEntry) => Number(!prerequisite(course) || prerequisite(course)!.complete);
  const notStarted = courses.filter((course) => !course.started).sort((a, b) => ready(b) - ready(a)).slice(0, 4);

  const pick = dailyPick({ tracks, progress, reviews, userId: user.id, now, skipExerciseId: nextUp?.lesson.exercise.id });
  const level = levelFor(user.xp);
  const xpFill = level.next === null ? 1 : user.xp / level.next;

  return (
    <div className="flex flex-col gap-6">
      {!user.timezone && <TimezoneSync />}
      <div className="flex flex-wrap items-stretch gap-6">
        {nextUp ? (
          <section aria-label="Resume course" className="panel flex min-w-0 flex-[999_1_600px] flex-col justify-between gap-10 p-6 sm:p-8">
            <div className="flex flex-col">
              <p className="text-sm text-muted">
                {nextUp.track.title} <span className="text-line-strong">/</span> Lesson {nextUp.track.lessons.indexOf(nextUp.lesson) + 1} of {nextUp.track.lessons.length} <span className="text-line-strong">/</span> {LANGUAGE_LABEL[nextUp.track.language]}, {nextUp.track.level}
              </p>
              <h1 className="display mt-3.5 text-3xl font-bold leading-[1.1] sm:text-[34px]">{nextUp.lesson.title}</h1>
              <p className="mt-3 max-w-[560px] text-base leading-relaxed text-text-2">{nextUp.lesson.summary}</p>
            </div>
            <div className="flex flex-wrap items-center gap-x-8 gap-y-6">
              <LessonPath track={nextUp.track} progress={progress} currentSlug={nextUp.lesson.slug} />
              <Link
                href={lessonPath(nextUp.track.slug, nextUp.lesson.slug)}
                className="inline-flex min-h-12 flex-none items-center gap-3 rounded-lg bg-accent px-5 text-[15px] font-semibold text-accent-ink hover:brightness-110 active:translate-y-px"
              >
                {progress.size ? "Open lesson" : "Start first lesson"} <span className="font-medium opacity-70">{nextUp.lesson.minutes} min</span>
              </Link>
            </div>
          </section>
        ) : (
          <section aria-label="All lessons passed" className="panel flex min-w-0 flex-[999_1_600px] flex-col justify-between gap-10 p-6 sm:p-8">
            <div>
              <h1 className="display text-3xl font-bold leading-[1.1] sm:text-[34px]">Every lesson passed.</h1>
              <p className="mt-3 max-w-[560px] text-base leading-relaxed text-text-2">Revisit a course, try a different solution, or build something of your own.</p>
            </div>
            <div><ButtonLink href="/learn" size="lg">Review courses</ButtonLink></div>
          </section>
        )}

        <aside aria-label="Your progress" className="panel flex min-w-0 flex-[1_1_320px] flex-col p-6 sm:p-8">
          <span className="text-sm font-semibold text-accent">Level {level.level}</span>
          <span className="display mt-1.5 text-[38px] font-bold leading-[1.05]">{level.title}</span>
          <span className="mt-2 text-sm text-muted">{titleBlurb(level.level)}</span>
          <div role="img" aria-label={level.next === null ? `${user.xp} XP, top level` : `${user.xp} of ${level.next} XP`} className="mt-7 h-2.5 overflow-hidden rounded-sm bg-line">
            <div className="h-full bg-accent" style={{ width: `${Math.round(Math.min(1, xpFill) * 100)}%` }} />
          </div>
          <div className="mt-3 flex justify-between gap-2 text-sm">
            <span className="font-medium">{user.xp}{level.next !== null && ` / ${level.next}`} XP</span>
            <span className="text-muted">{level.next === null ? "Top level. Challenges still earn XP." : `${level.next - user.xp} XP to level ${level.level + 1}`}</span>
          </div>
          <dl className="mt-auto flex flex-col pt-10">
            <div className="flex items-baseline justify-between border-t border-line py-4">
              <dt className="text-muted">Lessons passed</dt>
              <dd><span className="display text-2xl font-bold text-accent">{passedCount}</span> <span className="text-muted">of {allLessons.length}</span></dd>
            </div>
            <div className="flex items-baseline justify-between border-t border-line pt-4">
              <dt className="text-muted">Courses completed</dt>
              <dd><span className="display text-2xl font-bold">{completedCount}</span> <span className="text-muted">of {tracks.length}</span></dd>
            </div>
          </dl>
        </aside>
      </div>

      <div className="grid gap-6 md:grid-cols-2 xl:grid-cols-3">
        <RhythmCard rhythm={ctx.rhythm} />
        <ReviewCard due={dueReviews} />
        {pick && <PickCard pick={pick} />}
      </div>

      <CourseMap tracks={tracks} progress={progress} />

      {alsoActive.length > 0 && (
        <CoursePanel id="also-active" title="In progress" count={alsoActive.length}>
          {alsoActive.map(({ track, done, next }) => (
            <CourseLink key={track.slug} track={track} href={lessonPath(track.slug, next.slug)} note={`${done} of ${track.lessons.length} lessons complete`} action="Continue" />
          ))}
        </CoursePanel>
      )}

      {notStarted.length > 0 && (
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
      )}

      <CodexPanel unlocked={unlocked} />

      <StatsPanel
        months={months}
        seals={{ passed: ctx.passed.size, unaided: [...ctx.passed.values()].filter((p) => p.unaided).length, challenge: ctx.challenges.size }}
        firstTry={[...ctx.passed.values()].filter((p) => p.attemptsAtPass === 1).length}
        hints={{ recent: avgHints(recentDocs), earlier: earlierDocs.length ? avgHints(earlierDocs) : null }}
        tracks={tracks.map((t) => ({ title: t.title, done: t.lessons.filter((l) => ctx.passed.has(l.exercise.id)).length, total: t.lessons.length }))}
      />

      {!user.username && <UsernameNotice userId={user.id} />}
    </div>
  );
}

/** The course as a row of numbered steps: done, the one up next, and what's still to come. */
function LessonPath({ track, progress, currentSlug }: { track: Track; progress: Map<string, ProgressDoc>; currentSlug: string }) {
  const passed = track.lessons.map((lesson) => progress.get(lesson.exercise.id)?.status === "passed");
  const current = track.lessons.findIndex((lesson) => lesson.slug === currentSlug);
  // Long courses get small unnumbered dots so the path still fits beside the button.
  const dense = track.lessons.length > 8;
  return (
    <ol aria-label={`Lessons in this course, ${passed.filter(Boolean).length} of ${track.lessons.length} complete`} className="m-0 flex min-w-0 flex-[1_1_280px] list-none items-center p-0">
      {track.lessons.map((lesson, i) => {
        const done = passed[i];
        const isCurrent = i === current;
        const last = i === track.lessons.length - 1;
        const label = `Lesson ${i + 1}, ${done ? "complete" : isCurrent ? "up next" : "not started"}`;
        const tone = done ? "border-accent bg-accent text-accent-ink" : isCurrent ? "border-accent text-accent" : "border-line-strong text-muted";
        return (
          <li key={lesson.slug} aria-label={label} aria-current={isCurrent ? "step" : undefined} className={`flex items-center ${last ? "flex-none" : "flex-[1_1_0]"}`}>
            <span className={`box-border flex flex-none items-center justify-center rounded-full border-2 text-[13px] font-semibold ${dense ? "size-4 sm:size-[18px]" : "size-7 sm:size-8"} ${tone}`}>
              {done ? (
                <svg width={dense ? 10 : 14} height={dense ? 10 : 14} viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="3.4" strokeLinecap="round" strokeLinejoin="round" aria-hidden="true"><polyline points="4 12.5 9.5 18 20 6.5" /></svg>
              ) : (
                !dense && i + 1
              )}
            </span>
            {!last && <span aria-hidden="true" className={`h-0.5 min-w-0 flex-1 rounded-[1px] sm:min-w-2 ${dense ? "mx-0.5 sm:mx-1" : "mx-1 sm:mx-1.5"} ${done && (passed[i + 1] || i + 1 === current) ? "bg-accent" : "bg-line-strong"}`} />}
          </li>
        );
      })}
    </ol>
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
  return (
    <Link href={href} className="group flex flex-wrap items-center gap-x-5 gap-y-3 rounded-[10px] px-5 py-4 hover:bg-panel-2">
      <span
        className={`flex h-11 w-12 flex-none items-center justify-center rounded-md border font-mono text-[13px] font-medium group-hover:border-accent ${recommended ? "border-accent bg-accent text-accent-ink" : "border-line-strong"}`}
      >
        {TAG[track.language]}
      </span>
      <span className="flex min-w-0 flex-[1_1_260px] flex-col gap-0.5">
        <span className="display-sm text-[17px] font-semibold">{track.title}</span>
        <span className={`text-sm ${recommended ? "text-accent" : "text-muted"}`}>{note}</span>
      </span>
      <span className="flex-[0_0_104px] text-sm text-text-2">{track.level}</span>
      <span className="flex-[0_0_88px] text-sm text-text-2">{track.lessons.length} lessons</span>
      <span className="flex-[0_0_40px] text-sm text-text-2">{Math.max(1, Math.round(minutes / 60))} h</span>
      <span className="inline-flex min-h-9 items-center justify-center rounded-md border border-line-strong px-4 text-sm font-medium text-text-2 group-hover:border-accent group-hover:text-accent">{action}</span>
    </Link>
  );
}
