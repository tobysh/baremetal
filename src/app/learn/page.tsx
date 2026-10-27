import type { Metadata } from "next";
import Link from "next/link";
import { Suspense } from "react";
import { CourseProgress } from "@/components/course-progress";
import { JsonLd } from "@/components/json-ld";
import { Badge, Card, Container, Skeleton } from "@/components/ui";
import { getCurrentUser } from "@/lib/auth";
import { getTracks } from "@/lib/curriculum";
import { groupByPath } from "@/lib/curriculum/paths";
import { getProgressMap } from "@/lib/progress";
import { SITE, absoluteUrl } from "@/lib/site";
import { LANGUAGE_LABEL } from "@/lib/curriculum/types";

export const metadata: Metadata = {
  title: "Courses: C, Rust, Go and assembly from beginner to kernel",
  description:
    "Hands-on courses in C, Rust, Go and x86-64 assembly, from your first program to pointers, Rust ownership, an HTTP server and a kernel. Every lesson has an exercise.",
  alternates: { canonical: "/learn" },
};

export default async function LearnPage({ searchParams }: PageProps<"/learn">) {
  const tracks = getTracks();
  const groups = groupByPath(tracks);
  return (
    <Container className="py-8 sm:py-12">
      <JsonLd
        data={{
          "@context": "https://schema.org",
          "@type": "ItemList",
          itemListElement: tracks.map((t, i) => ({
            "@type": "ListItem",
            position: i + 1,
            url: absoluteUrl(`/learn/${t.slug}`),
            name: t.title,
          })),
        }}
      />
      <Suspense fallback={null}>
        <Welcome searchParams={searchParams} />
      </Suspense>
      <h1 className="display text-3xl font-bold sm:text-4xl">Choose your next challenge.</h1>
      <p className="mt-3 max-w-2xl text-muted">
        Pick a path. Courses are in order within each one. New to programming? Start with <Link className="text-accent underline underline-offset-2" href="/learn/first-steps">Programming from Zero</Link>, then choose a language.
      </p>
      <nav aria-label="Paths" className="mt-8 flex flex-wrap gap-2 border-b border-border pb-6 text-sm">
        {groups.map((g) => (
          <a key={g.id} href={`#path-${g.id}`} className="inline-flex min-h-11 items-center rounded-lg bg-surface px-4 py-2 text-muted hover:bg-surface-2 hover:text-text">
            {g.title}
          </a>
        ))}
      </nav>
      {groups.map((g) => (
        <section key={g.id} id={`path-${g.id}`} aria-labelledby={`h-${g.id}`} className="mt-12 scroll-mt-20">
          <h2 id={`h-${g.id}`} className="display-sm text-2xl font-bold">
            {g.title}
          </h2>
          <p className="mt-1 text-muted">{g.blurb}</p>
          <ol className="mt-5 grid gap-5">
            {g.tracks.map((t, i) => (
              <Card as="li" key={t.slug} className="grid gap-6 p-5 sm:p-6 md:grid-cols-[minmax(0,1fr)_17rem] md:gap-10">
                <div>
                  <div className="flex flex-wrap items-center gap-2">
                    {g.tracks.length > 1 && (
                      <span className="font-mono text-sm text-muted">
                        {i + 1} of {g.tracks.length}
                      </span>
                    )}
                    <Badge>{t.level}</Badge>
                    <Badge>{LANGUAGE_LABEL[t.language]}</Badge>
                  </div>
                  <h3 className="mt-3 text-xl font-semibold">
                    <Link href={`/learn/${t.slug}`} className="hover:text-accent">
                      {t.title}
                    </Link>
                  </h3>
                  <p className="mt-2 text-muted">{t.summary}</p>
                  {t.prerequisite && (
                    <p className="mt-3 text-sm text-muted">
                      Before this: <Link href={`/learn/${t.prerequisite}`} className="text-accent underline underline-offset-2">{tracks.find((x) => x.slug === t.prerequisite)?.title}</Link>
                    </p>
                  )}
                </div>
                <div className="flex flex-col justify-between gap-4">
                  <p className="text-sm text-muted">
                    {t.lessons.length} lessons · about {Math.round(t.lessons.reduce((m, l) => m + l.minutes, 0) / 60 * 2) / 2} hours
                  </p>
                  <Suspense fallback={<Skeleton className="h-10 w-full" />}>
                    <TrackProgress slug={t.slug} />
                  </Suspense>
                </div>
              </Card>
            ))}
          </ol>
        </section>
      ))}
    </Container>
  );
}

async function Welcome({ searchParams }: { searchParams: PageProps<"/learn">["searchParams"] }) {
  const { welcome } = await searchParams;
  if (!welcome) return null;
  const user = await getCurrentUser();
  return (
    <div role="status" className="mb-8 rounded-xl border border-border bg-ok-soft p-4 text-sm">
      Welcome to {SITE.name}{user ? `, ${user.name}` : ""}! Your account is ready and your progress will be saved as you go.
    </div>
  );
}

async function TrackProgress({ slug }: { slug: string }) {
  const track = getTracks().find((t) => t.slug === slug)!;
  const user = await getCurrentUser();
  const progress = user ? await getProgressMap(user.id) : new Map();
  return <CourseProgress track={track} progress={progress} signedIn={!!user} />;
}
