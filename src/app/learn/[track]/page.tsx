import { SealRow } from "@/components/seals";
import { sealsFor } from "@/lib/rewards-context";
import type { ProgressDoc } from "@/lib/models";
import type { Metadata } from "next";
import Link from "next/link";
import { notFound } from "next/navigation";
import { Suspense } from "react";
import { CourseProgress } from "@/components/course-progress";
import { JsonLd } from "@/components/json-ld";
import { TrackSkeleton } from "@/components/skeletons";
import { DifficultyBadge } from "@/components/gamification";
import { Badge, Container, Skeleton } from "@/components/ui";
import { getCurrentUser } from "@/lib/auth";
import { getTrack, getTracks, lessonPath } from "@/lib/curriculum";
import { unfinishedLesson } from "@/lib/learning";
import { getProgressMap } from "@/lib/progress";
import { SITE, absoluteUrl } from "@/lib/site";
import { LANGUAGE_LABEL } from "@/lib/curriculum/types";
import { TYPE_FILTERS, TYPE_LABEL, lessonTypes, parseTypeFilter, trackTypeCounts, type LessonType } from "@/lib/curriculum/lesson-types";

export function generateStaticParams() {
  return getTracks().map((t) => ({ track: t.slug }));
}

export async function generateMetadata({ params }: PageProps<"/learn/[track]">): Promise<Metadata> {
  const track = getTrack((await params).track);
  if (!track) return {};
  return {
    title: `${track.title} — free ${LANGUAGE_LABEL[track.language]} course`,
    description: track.summary,
    alternates: { canonical: `/learn/${track.slug}` },
    openGraph: { title: track.title, description: track.summary, url: `/learn/${track.slug}` },
  };
}

export default function TrackPage({ params, searchParams }: PageProps<"/learn/[track]">) {
  return (
    <Suspense fallback={<TrackSkeleton />}>
      <TrackContent params={params} searchParams={searchParams} />
    </Suspense>
  );
}

async function TrackContent({ params, searchParams }: { params: PageProps<"/learn/[track]">["params"]; searchParams: PageProps<"/learn/[track]">["searchParams"] }) {
  const track = getTrack((await params).track);
  if (!track) notFound();
  const tracks = getTracks();
  const prereq = track.prerequisite ? tracks.find((t) => t.slug === track.prerequisite) : undefined;
  const totalMinutes = track.lessons.reduce((m, l) => m + l.minutes, 0);

  return (
    <Container className="py-8 sm:py-12">
      <JsonLd
        data={[
          {
            "@context": "https://schema.org",
            "@type": "Course",
            name: track.title,
            description: track.summary,
            url: absoluteUrl(`/learn/${track.slug}`),
            inLanguage: "en",
            educationalLevel: track.level,
            teaches: track.lessons.map((l) => l.title),
            provider: { "@type": "Organization", name: SITE.name, sameAs: SITE.url },
            isAccessibleForFree: true,
            offers: { "@type": "Offer", category: "Free", price: 0, priceCurrency: "GBP" },
            hasCourseInstance: { "@type": "CourseInstance", courseMode: "Online", courseWorkload: `PT${totalMinutes}M` },
            ...(prereq ? { coursePrerequisites: prereq.title } : {}),
          },
          {
            "@context": "https://schema.org",
            "@type": "BreadcrumbList",
            itemListElement: [
              { "@type": "ListItem", position: 1, name: "Courses", item: absoluteUrl("/learn") },
              { "@type": "ListItem", position: 2, name: track.title, item: absoluteUrl(`/learn/${track.slug}`) },
            ],
          },
        ]}
      />
      <nav aria-label="Breadcrumb" className="text-sm text-muted">
        <Link href="/learn" className="hover:text-text">
          Courses
        </Link>{" "}
        / <span aria-current="page">{track.title}</span>
      </nav>
      <div className="mt-8 grid grid-cols-[minmax(0,1fr)] gap-10 lg:grid-cols-[minmax(0,1fr)_20rem] lg:gap-14">
        <div>
          <div className="flex flex-wrap gap-2">
            <Badge>{track.level}</Badge>
            <Badge>{LANGUAGE_LABEL[track.language]}</Badge>
            <Badge>{track.lessons.length} lessons</Badge>
          </div>
          <h1 className="display mt-4 text-3xl font-bold sm:text-4xl">{track.title}</h1>
          <p className="mt-4 max-w-2xl text-lg text-muted">{track.description}</p>
          <section className="mt-6" aria-label="What you will learn"><h2 className="display-sm text-lg font-semibold">What you will be able to do</h2><ul className="mt-2 list-disc space-y-2 pl-5 text-text-2">{track.outcomes.map((outcome) => <li key={outcome}>{outcome}</li>)}</ul></section>
          {prereq && (
            <p className="mt-4 text-sm text-muted">
              Recommended preparation:{" "}
              <Link className="text-accent underline underline-offset-2" href={`/learn/${prereq.slug}`}>
                {prereq.title}
              </Link>
              . You can start here if you already know those ideas.
            </p>
          )}
          <div className="mt-6 panel p-5 lg:hidden">
            <Suspense fallback={<Skeleton className="h-24" />}>
              <CourseAction trackSlug={track.slug} />
            </Suspense>
          </div>
          <div className="display-sm mt-12 flex items-baseline justify-between gap-4"><h2 className="text-xl font-bold">Course syllabus</h2><span className="text-sm text-muted">{track.lessons.length} lessons</span></div>
          {/* The full syllabus is always rendered (indexable, cacheable); only this small part reads `?type=` and hides rows. */}
          <Suspense fallback={<TypeFilter trackSlug={track.slug} type={null} />}>
            <TypeFilterFromParams trackSlug={track.slug} searchParams={searchParams} />
          </Suspense>
          <Suspense fallback={<LessonList trackSlug={track.slug} />}>
            <LessonListWithProgress trackSlug={track.slug} />
          </Suspense>
        </div>
        <aside className="hidden lg:block">
          <div className="sticky top-24 panel p-6">
            <h2 className="display-sm text-lg font-bold">Build your understanding.</h2>
            <p className="mb-6 mt-2 text-sm text-muted">About {Math.round(totalMinutes / 30) / 2} hours of hands-on practice, at your own pace.</p>
            <Suspense fallback={<Skeleton className="h-24" />}>
              <CourseAction trackSlug={track.slug} />
            </Suspense>
            <div className="mt-6 space-y-3 border-t border-border pt-5 text-sm text-muted"><p>✓ Real code in your browser</p><p>✓ Instant test feedback</p><p>✓ A tutor to help you think it through</p></div>
          </div>
        </aside>
      </div>
    </Container>
  );
}

const TYPE_ICON: Record<LessonType, string> = { lesson: "</>", review: "↻", challenge: "★", parsons: "☰", blanks: "▭", predict: "▶", bug: "⚑", choice: "◉", typed: "⌨", match: "⇄" };

async function TypeFilterFromParams({ trackSlug, searchParams }: { trackSlug: string; searchParams: PageProps<"/learn/[track]">["searchParams"] }) {
  return <TypeFilter trackSlug={trackSlug} type={parseTypeFilter((await searchParams).type)} />;
}

/** Chip row (`?type=`) plus the CSS that hides lessons of other types. Hidden types with no lessons are not offered. */
function TypeFilter({ trackSlug, type }: { trackSlug: string; type: LessonType | null }) {
  const track = getTrack(trackSlug)!;
  const counts = trackTypeCounts(track);
  const offered = TYPE_FILTERS.filter((t) => counts[t] > 0 || t === type);
  const hidden = type ? track.lessons.filter((l) => !lessonTypes(l).includes(type)) : [];
  const css = hidden.map((l) => `[data-lesson="${l.slug}"]{display:none}`).join("");
  if (!type && !TYPE_FILTERS.some((t) => t !== "lesson" && counts[t] > 0)) return null;
  const chip = (active: boolean) => `inline-flex min-h-11 shrink-0 items-center rounded-lg border px-3 py-2 text-sm ${active ? "border-accent bg-accent-soft text-text" : "border-border text-muted hover:bg-surface-2 hover:text-text"}`;
  const href = (t: LessonType | null) => (t ? `/learn/${trackSlug}?type=${t}` : `/learn/${trackSlug}`);
  return (
    <section aria-label="Filter lessons" className="mt-4">
      {css && <style>{css}</style>}
      <div role="group" aria-label="Lesson type" className="-mx-1 flex items-center gap-2 overflow-x-auto px-1 pb-1 sm:flex-wrap">
        <span className="shrink-0 text-sm text-muted">Type</span>
        <Link href={href(null)} aria-current={!type ? "true" : undefined} className={chip(!type)} scroll={false}>All</Link>
        {offered.map((t) => <Link key={t} href={href(t)} aria-current={type === t ? "true" : undefined} className={chip(type === t)} scroll={false}>{TYPE_LABEL[t]}</Link>)}
      </div>
      {type && hidden.length === track.lessons.length && <p role="status" className="mt-3 text-sm text-muted">No lessons in this course are of that type. <Link href={href(null)} className="text-accent underline underline-offset-2">Show all lessons</Link>.</p>}
    </section>
  );
}

function LessonList({ trackSlug, progress }: { trackSlug: string; progress?: Map<string, ProgressDoc> }) {
  const track = getTrack(trackSlug)!;
  // Only for learners with some progress: a brand-new learner just sees the list from lesson 1.
  const upNext = progress && track.lessons.some((l) => progress.has(l.exercise.id)) ? unfinishedLesson(track, progress)?.slug : undefined;
  return (
    <ol className="mt-4 divide-y divide-border overflow-hidden panel">
      {track.lessons.map((l, i) => {
        const done = progress?.get(l.exercise.id)?.status === "passed";
        const seals = progress ? sealsFor(progress, l.exercise.id) : null;
        return (
          <li key={l.slug} data-lesson={l.slug}>
            <Link href={lessonPath(track.slug, l.slug)} aria-current={l.slug === upNext ? "step" : undefined} className={`flex items-start gap-4 p-5 hover:bg-surface-2${l.slug === upNext ? " bg-accent-soft" : ""}`}>
              <span
                className={`mt-0.5 flex h-7 w-7 shrink-0 items-center justify-center rounded-full border font-mono text-xs ${done ? "border-transparent bg-ok-soft text-ok" : "border-border text-muted"}`}
                aria-hidden="true"
              >
                {done ? "✓" : i + 1}
              </span>
              <span className="min-w-0 flex-1">
                <span className="font-medium">{l.title}</span>
                {done && <span className="ml-2 text-xs font-medium text-ok">Completed</span>}
                {l.slug === upNext && <span className="ml-2 text-xs font-medium text-accent">Up next</span>}
                <span className="mt-1 block text-sm text-muted">{l.summary}</span>
                <span className="mt-2 flex flex-wrap gap-1.5">
                  {lessonTypes(l).map((t) => (
                    <span key={t} className={`inline-flex items-center gap-1 rounded-md border px-2 py-0.5 text-xs ${t === "review" ? "border-accent bg-accent-soft text-text" : "border-border text-muted"}`}>
                      <span aria-hidden="true">{TYPE_ICON[t]}</span>
                      {TYPE_LABEL[t]}
                    </span>
                  ))}
                </span>
                {l.tags.length > 0 && (
                  <span className="mt-2 flex flex-wrap gap-1.5">
                    {l.tags.map((t) => (
                      <span key={t} className="rounded-md bg-surface-2 px-2 py-0.5 font-mono text-xs text-muted">
                        #{t}
                      </span>
                    ))}
                  </span>
                )}
              </span>
              <span className="flex shrink-0 flex-col items-end gap-1">
                <DifficultyBadge difficulty={l.difficulty} />
                {seals ? <SealRow seals={seals} hasChallenge={!!l.challenge} /> : l.challenge && <span className="text-xs text-muted">Has a challenge</span>}
                <span className="text-xs text-muted">{l.minutes} min</span>
              </span>
            </Link>
          </li>
        );
      })}
    </ol>
  );
}

async function LessonListWithProgress({ trackSlug }: { trackSlug: string }) {
  const user = await getCurrentUser();
  if (!user) return <LessonList trackSlug={trackSlug} />;
  const progress = await getProgressMap(user.id);
  return <LessonList trackSlug={trackSlug} progress={progress} />;
}

async function CourseAction({ trackSlug }: { trackSlug: string }) {
  const track = getTrack(trackSlug)!;
  const user = await getCurrentUser();
  const progress = user ? await getProgressMap(user.id) : new Map();
  return <><CourseProgress track={track} progress={progress} signedIn={!!user} />{!user && <p className="mt-3 text-xs text-muted">Free. Sign in to save progress and use the AI tutor.</p>}</>;
}
