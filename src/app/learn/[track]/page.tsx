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
import { getProgressMap } from "@/lib/progress";
import { SITE, absoluteUrl } from "@/lib/site";
import { LANGUAGE_LABEL } from "@/lib/curriculum/types";

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

export default function TrackPage({ params }: PageProps<"/learn/[track]">) {
  return (
    <Suspense fallback={<TrackSkeleton />}>
      <TrackContent params={params} />
    </Suspense>
  );
}

async function TrackContent({ params }: { params: PageProps<"/learn/[track]">["params"] }) {
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
      <div className="mt-8 grid gap-10 lg:grid-cols-[minmax(0,1fr)_20rem] lg:gap-14">
        <div>
          <div className="flex flex-wrap gap-2">
            <Badge>{track.level}</Badge>
            <Badge>{LANGUAGE_LABEL[track.language]}</Badge>
            <Badge>{track.lessons.length} lessons</Badge>
          </div>
          <h1 className="display mt-4 text-3xl font-bold sm:text-4xl">{track.title}</h1>
          <p className="mt-4 max-w-2xl text-lg text-muted">{track.description}</p>
          {prereq && (
            <p className="mt-4 text-sm text-muted">
              Builds on{" "}
              <Link className="text-accent underline underline-offset-2" href={`/learn/${prereq.slug}`}>
                {prereq.title}
              </Link>
              .
            </p>
          )}
          <div className="mt-6 panel p-5 lg:hidden">
            <Suspense fallback={<Skeleton className="h-24" />}>
              <CourseAction trackSlug={track.slug} />
            </Suspense>
          </div>
          <div className="display-sm mt-12 flex items-baseline justify-between gap-4"><h2 className="text-xl font-bold">Course syllabus</h2><span className="text-sm text-muted">{track.lessons.length} lessons</span></div>
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

function LessonList({ trackSlug, passed }: { trackSlug: string; passed?: Set<string> }) {
  const track = getTrack(trackSlug)!;
  return (
    <ol className="mt-4 divide-y divide-border overflow-hidden panel">
      {track.lessons.map((l, i) => {
        const done = passed?.has(l.exercise.id);
        return (
          <li key={l.slug}>
            <Link href={lessonPath(track.slug, l.slug)} className="flex items-start gap-4 p-5 hover:bg-surface-2">
              <span
                className={`mt-0.5 flex h-7 w-7 shrink-0 items-center justify-center rounded-full border font-mono text-xs ${done ? "border-transparent bg-ok-soft text-ok" : "border-border text-muted"}`}
                aria-hidden="true"
              >
                {done ? "✓" : i + 1}
              </span>
              <span className="min-w-0 flex-1">
                <span className="font-medium">{l.title}</span>
                {l.kind === "review" && (
                  <span className="ml-2 align-middle">
                    <Badge tone="accent">Review</Badge>
                  </span>
                )}
                {done && <span className="ml-2 text-xs font-medium text-ok">Completed</span>}
                <span className="mt-1 block text-sm text-muted">{l.summary}</span>
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
  const passed = new Set([...progress.values()].filter((p) => p.status === "passed").map((p) => p.exerciseId));
  return <LessonList trackSlug={trackSlug} passed={passed} />;
}

async function CourseAction({ trackSlug }: { trackSlug: string }) {
  const track = getTrack(trackSlug)!;
  const user = await getCurrentUser();
  const progress = user ? await getProgressMap(user.id) : new Map();
  return <><CourseProgress track={track} progress={progress} signedIn={!!user} />{!user && <p className="mt-3 text-xs text-muted">Free. Sign in to save progress and use the AI tutor.</p>}</>;
}
