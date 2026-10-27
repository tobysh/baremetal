import type { Metadata } from "next";
import Link from "next/link";
import { Suspense } from "react";
import { LevelProgress } from "@/components/gamification";
import { ButtonLink, Card, Container, ProgressBar, Skeleton } from "@/components/ui";
import { requireUser } from "@/lib/auth";
import { getTracks, lessonPath } from "@/lib/curriculum";
import { getProgressMap } from "@/lib/progress";

export const metadata: Metadata = {
  title: "Your dashboard",
  description: "Your progress through the Baremetal C and Rust courses.",
  robots: { index: false },
};

export default function DashboardPage() {
  return (
    <Container className="py-12">
      <h1 className="text-3xl font-bold tracking-tight">Your dashboard</h1>
      <Suspense fallback={<DashboardSkeleton />}>
        <DashboardBody />
      </Suspense>
    </Container>
  );
}

async function DashboardBody() {
  const user = await requireUser("/dashboard");
  const progress = await getProgressMap(user.id);
  const tracks = getTracks();
  const allLessons = tracks.flatMap((t) => t.lessons.map((l) => ({ track: t, lesson: l })));
  const passedCount = allLessons.filter(({ lesson }) => progress.get(lesson.exercise.id)?.status === "passed").length;
  const attempts = [...progress.values()].reduce((n, p) => n + (p.attempts ?? 0), 0);

  const recent = [...progress.values()].sort((a, b) => +b.updatedAt - +a.updatedAt)[0];
  const recentLesson = recent && allLessons.find(({ lesson }) => lesson.exercise.id === recent.exerciseId);
  const nextUp = recentLesson && progress.get(recentLesson.lesson.exercise.id)?.status !== "passed"
    ? recentLesson
    : allLessons.find(({ lesson }) => progress.get(lesson.exercise.id)?.status !== "passed");

  return (
    <>
      <p className="mt-2 text-muted">Welcome back, {user.name}.</p>
      {!user.username && (
        <div role="status" className="mt-6 flex flex-wrap items-center justify-between gap-3 rounded-xl border border-accent/40 bg-accent-soft p-4 text-sm">
          <span>Pick a username to get a profile page and appear on the leaderboard.</span>
          <Link href="/settings" className="inline-flex h-9 items-center rounded-lg bg-accent px-3 font-medium text-accent-ink">
            Choose a username
          </Link>
        </div>
      )}
      <Card className="mt-6 p-6">
        <LevelProgress xp={user.xp} />
        <div className="mt-4 flex flex-wrap gap-4 text-sm">
          {user.username && (
            <Link href={`/u/${user.username}`} className="text-accent underline underline-offset-2">
              View your public profile
            </Link>
          )}
          <Link href="/leaderboard" className="text-accent underline underline-offset-2">
            Leaderboard
          </Link>
        </div>
      </Card>
      <div className="mt-6 grid gap-4 sm:grid-cols-3">
        <Card className="p-5">
          <p className="text-sm text-muted">Lessons completed</p>
          <p className="mt-1 text-3xl font-semibold">
            {passedCount}
            <span className="text-base font-normal text-muted"> / {allLessons.length}</span>
          </p>
        </Card>
        <Card className="p-5">
          <p className="text-sm text-muted">Test runs</p>
          <p className="mt-1 text-3xl font-semibold">{attempts}</p>
        </Card>
        <Card className="p-5">
          <p className="text-sm text-muted">Account security</p>
          <p className="mt-1 text-sm">
            {user.twoFactorEnabled ? (
              <span className="text-ok">Two-factor authentication is on</span>
            ) : (
              <Link href="/settings/security" className="text-accent underline underline-offset-2">
                Turn on two-factor authentication
              </Link>
            )}
          </p>
        </Card>
      </div>

      {nextUp ? (
        <Card className="mt-6 flex flex-col gap-4 p-6 sm:flex-row sm:items-center sm:justify-between">
          <div>
            <h2 className="text-sm font-medium text-muted">{recentLesson === nextUp ? "Pick up where you left off" : "Up next"}</h2>
            <p className="mt-1 text-lg font-semibold">{nextUp.lesson.title}</p>
            <p className="text-sm text-muted">{nextUp.track.title}</p>
          </div>
          <ButtonLink href={lessonPath(nextUp.track.slug, nextUp.lesson.slug)} size="lg">
            Continue
          </ButtonLink>
        </Card>
      ) : (
        <Card className="mt-6 p-6">
          <h2 className="text-lg font-semibold">You&apos;ve finished every lesson. Seriously impressive.</h2>
          <p className="mt-1 text-muted">Try rewriting your HTTP server with a thread pool, or revisit any lesson to experiment.</p>
        </Card>
      )}

      <h2 className="mt-12 text-xl font-semibold">Courses</h2>
      <ul className="mt-4 grid gap-4 md:grid-cols-2">
        {tracks.map((t) => {
          const done = t.lessons.filter((l) => progress.get(l.exercise.id)?.status === "passed").length;
          return (
            <Card as="li" key={t.slug} className="p-5">
              <div className="flex items-baseline justify-between gap-4">
                <h3 className="font-semibold">
                  <Link href={`/learn/${t.slug}`} className="hover:text-accent">
                    {t.title}
                  </Link>
                </h3>
                <span className="text-sm text-muted">
                  {done}/{t.lessons.length}
                </span>
              </div>
              <div className="mt-3">
                <ProgressBar value={done / t.lessons.length} label={`${t.title} progress`} />
              </div>
            </Card>
          );
        })}
      </ul>
    </>
  );
}

function DashboardSkeleton() {
  return (
    <div aria-busy="true" aria-label="Loading your dashboard">
      <Skeleton className="mt-3 h-5 w-48" />
      <div className="mt-8 grid gap-4 sm:grid-cols-3">
        <Skeleton className="h-24" />
        <Skeleton className="h-24" />
        <Skeleton className="h-24" />
      </div>
      <Skeleton className="mt-6 h-28" />
      <div className="mt-12 grid gap-4 md:grid-cols-2">
        {[0, 1, 2, 3].map((i) => (
          <Skeleton key={i} className="h-20" />
        ))}
      </div>
    </div>
  );
}
