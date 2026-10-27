import type { Metadata } from "next";
import Link from "next/link";
import { Suspense } from "react";
import { JsonLd } from "@/components/json-ld";
import { Badge, Card, Container, ProgressBar, Skeleton } from "@/components/ui";
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
    <Container className="py-12">
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
      <h1 className="text-3xl font-bold tracking-tight sm:text-4xl">Courses</h1>
      <p className="mt-3 max-w-2xl text-muted">
        Pick a path. Courses are in order within each one. New to programming? Start with <Link className="text-accent underline underline-offset-2" href="/learn/first-steps">Programming from Zero</Link>, then choose a language.
      </p>
      <nav aria-label="Paths" className="mt-6 flex flex-wrap gap-2 text-sm">
        {groups.map((g) => (
          <a key={g.id} href={`#path-${g.id}`} className="rounded-lg border border-border bg-surface px-3 py-1.5 hover:bg-surface-2">
            {g.title}
          </a>
        ))}
      </nav>
      {groups.map((g) => (
        <section key={g.id} id={`path-${g.id}`} aria-labelledby={`h-${g.id}`} className="mt-12 scroll-mt-20">
          <h2 id={`h-${g.id}`} className="text-2xl font-semibold tracking-tight">
            {g.title}
          </h2>
          <p className="mt-1 text-muted">{g.blurb}</p>
          <ol className="mt-5 grid gap-5">
            {g.tracks.map((t, i) => (
              <Card as="li" key={t.slug} className="grid gap-6 p-6 md:grid-cols-[1fr_16rem]">
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
                  <p className="mt-2 text-muted">{t.description}</p>
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
  const first = `/learn/${slug}/${track.lessons[0].slug}`;
  if (!user) {
    return (
      <Link href={first} className="inline-flex h-10 items-center justify-center rounded-lg border border-border bg-surface px-4 text-sm font-medium hover:bg-surface-2">
        Start course
      </Link>
    );
  }
  const progress = await getProgressMap(user.id);
  const done = track.lessons.filter((l) => progress.get(l.exercise.id)?.status === "passed").length;
  const next = track.lessons.find((l) => progress.get(l.exercise.id)?.status !== "passed") ?? track.lessons[0];
  return (
    <div className="space-y-3">
      <div className="flex justify-between text-sm">
        <span className="text-muted">Progress</span>
        <span>
          {done}/{track.lessons.length}
        </span>
      </div>
      <ProgressBar value={done / track.lessons.length} label={`${track.title} progress`} />
      <Link
        href={`/learn/${slug}/${next.slug}`}
        className="inline-flex h-10 w-full items-center justify-center rounded-lg bg-accent px-4 text-sm font-medium text-accent-ink hover:brightness-110"
      >
        {done === 0 ? "Start course" : done === track.lessons.length ? "Review course" : "Continue"}
      </Link>
    </div>
  );
}
