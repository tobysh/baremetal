import type { Metadata } from "next";
import Link from "next/link";
import { notFound } from "next/navigation";
import { Suspense } from "react";
import { JsonLd } from "@/components/json-ld";
import { LessonSkeleton } from "@/components/skeletons";
import { Markdown } from "@/components/markdown";
import { DifficultyBadge } from "@/components/gamification";
import { Badge, Skeleton } from "@/components/ui";
import { LessonWorkspace } from "@/components/workspace/lesson-workspace";
import { SealRow } from "@/components/seals";
import { CookieSettingsButton } from "@/components/consent";
import { LessonJump } from "@/components/workspace/lesson-jump";
import { LessonLayout } from "@/components/workspace/lesson-layout";
import { getCurrentUser } from "@/lib/auth";
import { getLesson, getTracks, lessonPath } from "@/lib/curriculum";
import { toPublicExercise } from "@/lib/curriculum/types";
import { challengeIdOf, challengeXpFor } from "@/lib/gamification";
import { sealsFor } from "@/lib/rewards-context";
import { recommendLearning } from "@/lib/learning";
import { getProgress, getProgressMap, getReviewFor, getTutorThread } from "@/lib/progress";
import { SITE, absoluteUrl } from "@/lib/site";
import { LANGUAGE_LABEL } from "@/lib/curriculum/types";

export function generateStaticParams() {
  return getTracks().flatMap((t) => t.lessons.map((l) => ({ track: t.slug, lesson: l.slug })));
}

export async function generateMetadata({ params }: PageProps<"/learn/[track]/[lesson]">): Promise<Metadata> {
  const { track: t, lesson: l } = await params;
  const found = getLesson(t, l);
  if (!found) return {};
  const { track, lesson, index } = found;
  const path = lessonPath(track.slug, lesson.slug);
  return {
    title: `${lesson.title} — ${track.title}, lesson ${index + 1}`,
    description: lesson.summary,
    alternates: { canonical: path },
    openGraph: { type: "article", title: `${lesson.title} · ${track.title}`, description: lesson.summary, url: path },
  };
}

export default function LessonPage({ params, searchParams }: PageProps<"/learn/[track]/[lesson]">) {
  // URL data is read inside the boundary so client navigations between lessons
  // can show the skeleton immediately. Generated lesson paths are still fully
  // prerendered at build time.
  return (
    <Suspense fallback={<LessonSkeleton />}>
      <Lesson params={params} searchParams={searchParams} />
    </Suspense>
  );
}

async function Lesson({ params, searchParams }: { params: PageProps<"/learn/[track]/[lesson]">["params"]; searchParams: PageProps<"/learn/[track]/[lesson]">["searchParams"] }) {
  const { track: t, lesson: l } = await params;
  const found = getLesson(t, l);
  if (!found) notFound();
  const { track, lesson, index, prev, next } = found;
  const path = lessonPath(track.slug, lesson.slug);
  const exercise = toPublicExercise(lesson.exercise);

  return (
    <div className="mx-auto w-full max-w-[120rem] px-4 py-6 sm:px-6 lg:flex lg:h-[calc(100dvh-4rem-1px)] lg:flex-col lg:px-8 lg:py-3">
      <JsonLd
        data={[
          {
            "@context": "https://schema.org",
            "@type": "LearningResource",
            name: lesson.title,
            description: lesson.summary,
            url: absoluteUrl(path),
            learningResourceType: "Lesson with coding exercise",
            educationalLevel: track.level,
            inLanguage: "en",
            timeRequired: `PT${lesson.minutes}M`,
            ...(lesson.tags.length ? { keywords: lesson.tags.join(", ") } : {}),
            isAccessibleForFree: true,
            programmingLanguage: LANGUAGE_LABEL[track.language],
            isPartOf: { "@type": "Course", name: track.title, url: absoluteUrl(`/learn/${track.slug}`) },
            provider: { "@type": "Organization", name: SITE.name, url: SITE.url },
          },
          {
            "@context": "https://schema.org",
            "@type": "BreadcrumbList",
            itemListElement: [
              { "@type": "ListItem", position: 1, name: "Courses", item: absoluteUrl("/learn") },
              { "@type": "ListItem", position: 2, name: track.title, item: absoluteUrl(`/learn/${track.slug}`) },
              { "@type": "ListItem", position: 3, name: lesson.title, item: absoluteUrl(path) },
            ],
          },
        ]}
      />
      <LessonLayout reading={
        <article className="min-w-0">
          <nav aria-label="Breadcrumb" className="text-sm text-muted">
            <Link href="/learn" className="hover:text-text">
              Courses
            </Link>{" "}
            /{" "}
            <Link href={`/learn/${track.slug}`} className="hover:text-text">
              {track.title}
            </Link>
          </nav>
          <div className="mt-4 flex flex-wrap items-center gap-2">
            <Badge>
              Lesson {index + 1} of {track.lessons.length}
            </Badge>
            <DifficultyBadge difficulty={lesson.difficulty} />
            <Badge>{lesson.minutes} min</Badge>
            {lesson.kind === "review" && <Badge tone="accent">Review</Badge>}
            <Suspense fallback={null}>
              <LessonSeals exerciseId={exercise.id} hasChallenge={!!lesson.challenge} />
            </Suspense>
          </div>
          {lesson.tags.length > 0 && (
            <ul aria-label="Topics" className="mt-2 flex flex-wrap gap-1.5">
              {lesson.tags.map((t) => (
                <li key={t} className="rounded-md bg-surface-2 px-2 py-0.5 font-mono text-xs text-muted">
                  #{t}
                </li>
              ))}
            </ul>
          )}
          <h1 className="display mt-3 text-3xl font-bold">{lesson.title}</h1>
          <p className="mt-2 text-muted">{lesson.summary}</p>
          <Markdown className="mt-6">{lesson.body}</Markdown>
          <section aria-labelledby="task" className="mt-10 panel p-5">
            <h2 id="task" className="display-sm text-lg font-bold">
              Your task
            </h2>
            <Markdown className="mt-3">{exercise.task}</Markdown>
            <LessonJump target="code" className="mt-4 inline-flex min-h-11 items-center rounded-lg bg-accent px-4 text-sm font-semibold text-accent-ink hover:brightness-110">
              Start coding →
            </LessonJump>
          </section>
          <nav aria-label="Lesson navigation" className="mt-10 flex justify-between gap-4 text-sm">
            {prev ? (
              <Link href={lessonPath(track.slug, prev.slug)} className="text-muted hover:text-text">
                ← {prev.title}
              </Link>
            ) : (
              <span />
            )}
            {next ? (
              <Link href={lessonPath(track.slug, next.slug)} className="text-right text-muted hover:text-text">
                {next.title} →
              </Link>
            ) : (
              <Link href="/learn" className="text-right text-muted hover:text-text">
                Back to all courses →
              </Link>
            )}
          </nav>
        </article>
      } workspace={
        <div className="min-w-0 lg:h-full">
          <Suspense fallback={<WorkspaceSkeleton starter={exercise.starter} />}>
            <WorkspaceLoader exerciseId={exercise.id} path={path} trackSlug={track.slug} lessonSlug={lesson.slug} searchParams={searchParams} />
          </Suspense>
        </div>
      } />
      <p className="mt-3 flex shrink-0 flex-wrap items-center justify-center gap-x-2 gap-y-1 border-t border-border pt-2 text-xs text-muted lg:mt-2">
        <span>© 2026 {SITE.legal.entity}</span>
        <span aria-hidden="true">-</span>
        <Link href="/terms" className="hover:text-text">Terms and conditions</Link>
        <span aria-hidden="true">-</span>
        <Link href="/privacy" className="hover:text-text">Privacy policy</Link>
        <span aria-hidden="true">-</span>
        <CookieSettingsButton />
      </p>
    </div>
  );
}

async function LessonSeals({ exerciseId, hasChallenge }: { exerciseId: string; hasChallenge: boolean }) {
  const user = await getCurrentUser();
  if (!user) return null;
  const progress = await getProgressMap(user.id);
  return <SealRow seals={sealsFor(progress, exerciseId)} hasChallenge={hasChallenge} size={22} />;
}

async function WorkspaceLoader({ exerciseId, path, trackSlug, lessonSlug, searchParams }: { exerciseId: string; path: string; trackSlug: string; lessonSlug: string; searchParams: PageProps<"/learn/[track]/[lesson]">["searchParams"] }) {
  const { lesson } = getLesson(trackSlug, lessonSlug)!;
  const user = await getCurrentUser();
  const wantReview = (await searchParams).review === "1";
  const [progress, thread, challengeProgress, review] = user
    ? await Promise.all([
        getProgress(user.id, exerciseId),
        getTutorThread(user.id, exerciseId),
        lesson.challenge ? getProgress(user.id, challengeIdOf(exerciseId)) : null,
        getReviewFor(user.id, exerciseId),
      ])
    : [null, null, null, null];
  const tracks = getTracks();
  const track = tracks.find((t) => t.slug === trackSlug)!;
  // Anonymous continuation follows the authored sequence, without claiming saved progress.
  const continuationProgress = user ? await getProgressMap(user.id) : new Map(track.lessons.slice(0, track.lessons.indexOf(lesson)).map((l) => [l.exercise.id, { exerciseId: l.exercise.id, status: "passed", updatedAt: new Date(0) }]));
  const continuation = recommendLearning(tracks, continuationProgress, { track, afterPass: exerciseId });
  const continuationHref = continuation ? lessonPath(continuation.track.slug, continuation.lesson.slug) : "/learn";
  const continuationLabel = continuation ? continuation.track.slug === trackSlug ? "Next lesson →" : `Next course: ${continuation.track.title} →` : "Choose another path →";
  return (
    <LessonWorkspace
      key={exerciseId}
      lesson={toPublicExercise(lesson.exercise)}
      lessonCode={progress?.code ?? null}
      lessonPassed={progress?.status === "passed"}
      initialHints={progress?.hintsUsed ?? 0}
      challenge={lesson.challenge ? { exercise: toPublicExercise(lesson.challenge.exercise), code: challengeProgress?.code ?? null, passed: challengeProgress?.status === "passed", xp: challengeXpFor(lesson.difficulty) } : null}
      review={review ? { due: review.dueAt <= new Date(), dueOn: review.dueAt.toLocaleDateString("en-GB", { day: "numeric", month: "short" }) } : null}
      startInReview={wantReview}
      signedIn={!!user}
      draftOwner={user?.id}
      tutorHistory={(thread?.messages ?? []).map((m) => ({ role: m.role, content: m.content }))}
      nextHref={continuationHref}
      nextLabel={continuationLabel}
      loginHref={`/login?next=${encodeURIComponent(path)}`}
    />
  );
}

function WorkspaceSkeleton({ starter }: { starter: string }) {
  return (
    <div className="flex flex-col overflow-hidden panel lg:h-full" aria-busy="true" aria-label="Loading the code editor">
      <div className="flex items-center gap-2 border-b border-border px-3 py-2">
        <Skeleton className="h-4 w-16" />
        <div className="ml-auto flex gap-2">
          <Skeleton className="h-8 w-14" />
          <Skeleton className="h-8 w-14" />
        </div>
      </div>
      <pre className="h-[22rem] overflow-auto border-b border-border p-4 font-mono text-sm leading-6 text-muted lg:h-auto lg:flex-[1.3]">{starter}</pre>
      <div className="space-y-2 p-4 lg:flex-1">
        <Skeleton className="h-4 w-1/3" />
        <Skeleton className="h-4 w-2/3" />
        <Skeleton className="h-4 w-1/2" />
      </div>
    </div>
  );
}
