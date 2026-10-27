import type { Metadata } from "next";
import Link from "next/link";
import { notFound } from "next/navigation";
import { Suspense } from "react";
import { JsonLd } from "@/components/json-ld";
import { LessonSkeleton } from "@/components/skeletons";
import { Markdown } from "@/components/markdown";
import { DifficultyBadge } from "@/components/gamification";
import { Badge, Skeleton } from "@/components/ui";
import { Workspace } from "@/components/workspace/workspace";
import { getCurrentUser } from "@/lib/auth";
import { getLesson, getTracks, lessonPath } from "@/lib/curriculum";
import { toPublicExercise } from "@/lib/curriculum/types";
import { getProgress, getTutorThread } from "@/lib/progress";
import { SITE, absoluteUrl } from "@/lib/site";

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

export default function LessonPage({ params }: PageProps<"/learn/[track]/[lesson]">) {
  // URL data is read inside the boundary so client navigations between lessons
  // can show the skeleton immediately. Generated lesson paths are still fully
  // prerendered at build time.
  return (
    <Suspense fallback={<LessonSkeleton />}>
      <Lesson params={params} />
    </Suspense>
  );
}

async function Lesson({ params }: { params: PageProps<"/learn/[track]/[lesson]">["params"] }) {
  const { track: t, lesson: l } = await params;
  const found = getLesson(t, l);
  if (!found) notFound();
  const { track, lesson, index, prev, next } = found;
  const path = lessonPath(track.slug, lesson.slug);
  const nextHref = next ? lessonPath(track.slug, next.slug) : null;
  const exercise = toPublicExercise(lesson.exercise);

  return (
    <div className="mx-auto w-full max-w-[96rem] px-4 py-6 sm:px-6">
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
            isAccessibleForFree: true,
            programmingLanguage: track.language === "c" ? "C" : "Rust",
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
      <div className="grid gap-6 lg:grid-cols-[minmax(0,1fr)_minmax(0,1.15fr)]">
        <article className="min-w-0 lg:max-h-[calc(100dvh-6.5rem)] lg:overflow-y-auto lg:pr-4">
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
          </div>
          <h1 className="mt-3 text-3xl font-bold tracking-tight">{lesson.title}</h1>
          <p className="mt-2 text-muted">{lesson.summary}</p>
          <Markdown className="mt-6">{lesson.body}</Markdown>
          <section aria-labelledby="task" className="mt-10 rounded-xl border border-border bg-surface p-5">
            <h2 id="task" className="text-lg font-semibold">
              Your task
            </h2>
            <Markdown className="mt-3">{exercise.task}</Markdown>
          </section>
          <nav aria-label="Lesson navigation" className="mt-8 flex justify-between gap-4 text-sm">
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
        <div className="min-w-0 lg:sticky lg:top-20 lg:self-start">
          <Suspense fallback={<WorkspaceSkeleton starter={exercise.starter} />}>
            <WorkspaceLoader exerciseId={exercise.id} path={path} nextHref={nextHref} trackSlug={track.slug} lessonSlug={lesson.slug} />
          </Suspense>
        </div>
      </div>
    </div>
  );
}

async function WorkspaceLoader({ exerciseId, path, nextHref, trackSlug, lessonSlug }: { exerciseId: string; path: string; nextHref: string | null; trackSlug: string; lessonSlug: string }) {
  const { lesson } = getLesson(trackSlug, lessonSlug)!;
  const user = await getCurrentUser();
  const [progress, thread] = user ? await Promise.all([getProgress(user.id, exerciseId), getTutorThread(user.id, exerciseId)]) : [null, null];
  return (
    <Workspace
      exercise={toPublicExercise(lesson.exercise)}
      initialCode={progress?.code ?? null}
      signedIn={!!user}
      alreadyPassed={progress?.status === "passed"}
      tutorHistory={(thread?.messages ?? []).map((m) => ({ role: m.role, content: m.content }))}
      nextHref={nextHref}
      loginHref={`/login?next=${encodeURIComponent(path)}`}
    />
  );
}

function WorkspaceSkeleton({ starter }: { starter: string }) {
  return (
    <div className="flex flex-col overflow-hidden rounded-xl border border-border bg-surface lg:h-[calc(100dvh-6.5rem)]" aria-busy="true" aria-label="Loading the code editor">
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
