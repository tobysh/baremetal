import type { Metadata } from "next";
import Link from "next/link";
import { Suspense } from "react";
import { JsonLd } from "@/components/json-ld";
import { Badge, Card, Container, ProgressBar, Skeleton } from "@/components/ui";
import { getCurrentUser } from "@/lib/auth";
import { getTracks } from "@/lib/curriculum";
import { getProgressMap } from "@/lib/progress";
import { SITE, absoluteUrl } from "@/lib/site";

export const metadata: Metadata = {
  title: "Courses: C and Rust from beginner to HTTP server",
  description:
    "Four hands-on courses that take you from your first program to pointers, memory, Rust ownership and a multi-threaded HTTP server. Every lesson has an exercise.",
  alternates: { canonical: "/learn" },
};

export default async function LearnPage({ searchParams }: PageProps<"/learn">) {
  const tracks = getTracks();
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
        Take them in order — each one builds on the last. New to programming? Start with <Link className="text-accent underline underline-offset-2" href="/learn/first-steps">Programming from Zero</Link>.
      </p>
      <ol className="mt-10 grid gap-5">
        {tracks.map((t, i) => (
          <Card as="li" key={t.slug} className="grid gap-6 p-6 md:grid-cols-[1fr_16rem]">
            <div>
              <div className="flex flex-wrap items-center gap-2">
                <span className="font-mono text-sm text-muted">Course {i + 1}</span>
                <Badge>{t.level}</Badge>
                <Badge>{t.language === "c" ? "C" : "Rust"}</Badge>
              </div>
              <h2 className="mt-3 text-xl font-semibold">
                <Link href={`/learn/${t.slug}`} className="hover:text-accent">
                  {t.title}
                </Link>
              </h2>
              <p className="mt-2 text-muted">{t.description}</p>
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
