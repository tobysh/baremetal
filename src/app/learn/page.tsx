import type { Metadata } from "next";
import Link from "next/link";
import { Suspense, cache } from "react";
import { CourseProgress } from "@/components/course-progress";
import { JsonLd } from "@/components/json-ld";
import { Badge, Card, Container, Skeleton } from "@/components/ui";
import { getCurrentUser } from "@/lib/auth";
import { getTracks } from "@/lib/curriculum";
import { recommendLearning } from "@/lib/learning";
import { groupByPath } from "@/lib/curriculum/paths";
import { getProgressMap } from "@/lib/progress";
import { SITE, absoluteUrl } from "@/lib/site";
import { LANGUAGE_LABEL, type Track } from "@/lib/curriculum/types";

export const metadata: Metadata = {
  title: "Courses: C, Rust, Go and assembly from beginner to kernel",
  description:
    "Hands-on courses in C, Rust, Go and x86-64 assembly, from your first program to pointers, Rust ownership, an HTTP server and a kernel. Every lesson has an exercise.",
  alternates: { canonical: "/learn" },
};

const LANG_FILTERS = [
  { id: "c", label: "C" }, { id: "rust", label: "Rust" }, { id: "go", label: "Go" },
  { id: "asm", label: "Assembly" }, { id: "kernel", label: "Kernel" }, { id: "security", label: "Security" },
] as const;
const LEVEL_FILTERS = ["Beginner", "Intermediate", "Advanced"] as const;

const matchesLang = (t: Track, lang: string) => (lang === "security" ? t.path === "security" : t.path !== "security" && t.language === lang);

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
        Pick a path. Courses are in order within each one. New to programming? Start with <Link className="text-accent underline underline-offset-2" href="/learn/first-steps">Programming from Zero</Link>, then continue into C. You can choose another path at any time.
      </p>
      <Suspense fallback={null}><RecommendedLearning /></Suspense>
      <Suspense fallback={<CourseFilterBar lang={null} level={null} />}><CourseFilters searchParams={searchParams} tracks={tracks} /></Suspense>
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
              <Card as="li" key={t.slug} data-track={t.slug} className="grid gap-6 p-5 sm:p-6 md:grid-cols-[minmax(0,1fr)_17rem] md:gap-10">
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
                  {/* On phones the outcome list is folded away so the catalogue stays short. */}
                  <details className="mt-3 md:hidden">
                    <summary className="min-h-11 cursor-pointer py-2 text-sm font-medium text-accent">What you&apos;ll learn</summary>
                    <ul className="list-disc space-y-1 pl-5 text-sm text-text-2">{t.outcomes.map((outcome) => <li key={outcome}>{outcome}</li>)}</ul>
                  </details>
                  <ul className="mt-3 hidden list-disc space-y-1 pl-5 text-sm text-text-2 md:block">{t.outcomes.map((outcome) => <li key={outcome}>{outcome}</li>)}</ul>
                  {t.prerequisite && (
                    <p className="mt-3 text-sm text-muted">
                      Recommended before this: <Link href={`/learn/${t.prerequisite}`} className="text-accent underline underline-offset-2">{tracks.find((x) => x.slug === t.prerequisite)?.title}</Link>
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

/**
 * Plain-link filters (`?lang=rust&level=Beginner`). The catalogue itself is always rendered in full so the page stays
 * cacheable and indexable; this small request-time part only emits CSS that hides what doesn't match, which works
 * without JavaScript.
 */
async function CourseFilters({ searchParams, tracks }: { searchParams: PageProps<"/learn">["searchParams"]; tracks: Track[] }) {
  const params = await searchParams;
  const one = (v: string | string[] | undefined) => (Array.isArray(v) ? v[0] : v) ?? null;
  const lang = LANG_FILTERS.find((l) => l.id === one(params.lang))?.id ?? null;
  const level = LEVEL_FILTERS.find((l) => l === one(params.level)) ?? null;
  const shown = tracks.filter((t) => (!lang || matchesLang(t, lang)) && (!level || t.level === level));
  const css = !lang && !level ? "" : [
    ...tracks.filter((t) => !shown.includes(t)).map((t) => `[data-track="${t.slug}"]{display:none}`),
    ...groupByPath(tracks).filter((g) => !g.tracks.some((t) => shown.includes(t))).map((g) => `#path-${g.id}{display:none}`),
  ].join("");
  return (
    <>
      {css && <style>{css}</style>}
      <CourseFilterBar lang={lang} level={level} empty={shown.length === 0} />
    </>
  );
}

function CourseFilterBar({ lang, level, empty = false }: { lang: string | null; level: string | null; empty?: boolean }) {
  const href = (nextLang: string | null, nextLevel: string | null) => {
    const q = new URLSearchParams();
    if (nextLang) q.set("lang", nextLang);
    if (nextLevel) q.set("level", nextLevel);
    return q.size ? `/learn?${q}` : "/learn";
  };
  const chip = (active: boolean) => `inline-flex min-h-11 items-center rounded-lg border px-3 py-2 text-sm ${active ? "border-accent bg-accent-soft text-text" : "border-border text-muted hover:bg-surface-2 hover:text-text"}`;
  return (
    <section aria-label="Filter courses" className="mt-8 space-y-3">
      <div className="flex flex-wrap items-center gap-2">
        <span className="text-sm text-muted">Language</span>
        <Link href={href(null, level)} aria-current={!lang ? "true" : undefined} className={chip(!lang)} scroll={false}>All</Link>
        {LANG_FILTERS.map((l) => <Link key={l.id} href={href(l.id, level)} aria-current={lang === l.id ? "true" : undefined} className={chip(lang === l.id)} scroll={false}>{l.label}</Link>)}
      </div>
      <div className="flex flex-wrap items-center gap-2">
        <span className="text-sm text-muted">Level</span>
        <Link href={href(lang, null)} aria-current={!level ? "true" : undefined} className={chip(!level)} scroll={false}>All</Link>
        {LEVEL_FILTERS.map((l) => <Link key={l} href={href(lang, l)} aria-current={level === l ? "true" : undefined} className={chip(level === l)} scroll={false}>{l}</Link>)}
      </div>
      {empty && <p role="status" className="text-sm text-muted">No courses match both filters. <Link href="/learn" className="text-accent underline underline-offset-2">Clear them</Link>.</p>}
    </section>
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

// One progress query per render instead of one per course card.
const progressFor = cache(async () => {
  const user = await getCurrentUser();
  return { user, progress: user ? await getProgressMap(user.id) : new Map<string, Awaited<ReturnType<typeof getProgressMap>> extends Map<string, infer V> ? V : never>() };
});

async function TrackProgress({ slug }: { slug: string }) {
  const track = getTracks().find((t) => t.slug === slug)!;
  const { user, progress } = await progressFor();
  return <CourseProgress track={track} progress={progress} signedIn={!!user} />;
}

async function RecommendedLearning() {
  const { progress } = await progressFor();
  const tracks = getTracks();
  const next = recommendLearning(tracks, progress);
  return <section aria-label="Recommended learning" className="mt-6 panel p-5">
    <h2 className="display-sm text-lg font-semibold">{progress.size ? "Continue learning" : "Your recommended starting point"}</h2>
    {next ? <>
      <p className="mt-2 text-muted">{next.track.title} · {next.lesson.title}</p>
      <Link className="mt-3 inline-flex min-h-11 items-center rounded-lg bg-accent px-4 font-medium text-accent-ink" href={`/learn/${next.track.slug}/${next.lesson.slug}`}>{progress.size ? "Continue learning" : "Start first lesson"} →</Link>
    </> : <p className="mt-2 text-muted">You&apos;ve finished this path. Choose another below.</p>}
    <p className="mt-3 text-sm text-muted">Recommendations are a guide. Every course remains open to you.</p>
  </section>;
}
