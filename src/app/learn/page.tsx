import type { Metadata } from "next";
import Link from "next/link";
import { Suspense, cache } from "react";
import { CourseProgress } from "@/components/course-progress";
import { JsonLd } from "@/components/json-ld";
import { Container, ProgressBar, Skeleton } from "@/components/ui";
import { getCurrentUser } from "@/lib/auth";
import { getTracks } from "@/lib/curriculum";
import { recommendLearning } from "@/lib/learning";
import { groupByPath } from "@/lib/curriculum/paths";
import { getProgressMap } from "@/lib/progress";
import { SITE, absoluteUrl } from "@/lib/site";
import { LANGUAGE_LABEL, type Track } from "@/lib/curriculum/types";
import { TYPE_FILTERS, TYPE_LABEL, parseTypeFilter, trackTypeCounts, typeCounts, type LessonType } from "@/lib/curriculum/lesson-types";

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
  const counts = typeCounts(tracks);
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
      <h1 className="sr-only">Courses</h1>
      <Suspense fallback={<Skeleton className="h-64 w-full" />}><RecommendedLearning /></Suspense>
      <h2 id="all-courses" className="display-sm mt-14 scroll-mt-20 text-2xl font-bold">All courses</h2>
      <p className="mt-1 max-w-2xl text-muted">Courses run in order within each path. Prerequisites are advice, never locks.</p>
      <Suspense fallback={<CourseFilterBar lang={null} level={null} type={null} counts={counts} tracks={tracks} />}><CourseFilters searchParams={searchParams} tracks={tracks} counts={counts} /></Suspense>
      <nav aria-label="Jump to a path" className="mt-1 flex flex-wrap items-baseline gap-x-4 text-sm text-muted">
        <span>Jump to:</span>
        {groups.map((g) => (
          <a key={g.id} href={`#path-${g.id}`} className="py-2 text-accent underline-offset-2 hover:underline">
            {g.title} ↓
          </a>
        ))}
      </nav>
      {groups.map((g) => (
        <section key={g.id} id={`path-${g.id}`} aria-labelledby={`h-${g.id}`} className="mt-12 scroll-mt-20">
          <h2 id={`h-${g.id}`} className="display-sm text-2xl font-bold">
            {g.title}
          </h2>
          <p className="mt-1 text-muted">{g.blurb}</p>
          <ol className="mt-4 divide-y divide-border border-y border-border">
            {g.tracks.map((t, i) => (
              <li key={t.slug} data-track={t.slug} className="grid gap-4 py-5 md:grid-cols-[minmax(0,1fr)_14rem] md:gap-10">
                <div>
                  <p className="flex flex-wrap items-center gap-x-2 text-xs text-muted">
                    {g.tracks.length > 1 && <span className="font-mono">{i + 1} of {g.tracks.length} ·</span>}
                    <span>{t.level}</span><span aria-hidden="true">·</span><span>{LANGUAGE_LABEL[t.language]}</span>
                    <span aria-hidden="true">·</span><span>{t.lessons.length} lessons, about {Math.round(t.lessons.reduce((m, l) => m + l.minutes, 0) / 60 * 2) / 2} h</span>
                  </p>
                  <h3 className="mt-1 text-lg font-semibold">
                    <Link href={`/learn/${t.slug}`} className="hover:text-accent">{t.title}</Link>
                  </h3>
                  <p className="mt-1 text-muted">{t.summary}</p>
                  <details className="mt-2">
                    <summary className="min-h-11 cursor-pointer py-2 text-sm font-medium text-accent">What you&apos;ll learn</summary>
                    <ul className="list-disc space-y-1 pl-5 text-sm text-text-2">{t.outcomes.map((outcome) => <li key={outcome}>{outcome}</li>)}</ul>
                    {t.prerequisite && (
                      <p className="mt-2 text-sm text-muted">
                        Recommended before this: <Link href={`/learn/${t.prerequisite}`} className="text-accent underline underline-offset-2">{tracks.find((x) => x.slug === t.prerequisite)?.title}</Link>
                      </p>
                    )}
                  </details>
                </div>
                <div className="flex flex-col justify-center">
                  <Suspense fallback={<Skeleton className="h-10 w-full" />}>
                    <TrackProgress slug={t.slug} />
                  </Suspense>
                </div>
              </li>
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
async function CourseFilters({ searchParams, tracks, counts }: { searchParams: PageProps<"/learn">["searchParams"]; tracks: Track[]; counts: Record<LessonType, number> }) {
  const params = await searchParams;
  const one = (v: string | string[] | undefined) => (Array.isArray(v) ? v[0] : v) ?? null;
  const lang = LANG_FILTERS.find((l) => l.id === one(params.lang))?.id ?? null;
  const level = LEVEL_FILTERS.find((l) => l === one(params.level)) ?? null;
  const type = parseTypeFilter(params.type);
  const shown = tracks.filter((t) => (!lang || matchesLang(t, lang)) && (!level || t.level === level) && (!type || trackTypeCounts(t)[type] > 0));
  const css = !lang && !level && !type ? "" : [
    ...tracks.filter((t) => !shown.includes(t)).map((t) => `[data-track="${t.slug}"]{display:none}`),
    ...groupByPath(tracks).filter((g) => !g.tracks.some((t) => shown.includes(t))).map((g) => `#path-${g.id}{display:none}`),
  ].join("");
  return (
    <>
      {css && <style>{css}</style>}
      <CourseFilterBar lang={lang} level={level} type={type} counts={counts} tracks={tracks} />
    </>
  );
}

function CourseFilterBar({ lang, level, type, counts, tracks }: { lang: string | null; level: string | null; type: LessonType | null; counts: Record<LessonType, number>; tracks: Track[] }) {
  const href = (nextLang: string | null, nextLevel: string | null, nextType: LessonType | null) => {
    const q = new URLSearchParams();
    if (nextLang) q.set("lang", nextLang);
    if (nextLevel) q.set("level", nextLevel);
    if (nextType) q.set("type", nextType);
    return q.size ? `/learn?${q}` : "/learn";
  };
  // How many courses a chip would show, given the other two filters, so no click leads to an empty page by surprise.
  const howMany = (l: string | null, v: string | null, ty: LessonType | null) =>
    tracks.filter((t) => (!l || matchesLang(t, l)) && (!v || t.level === v) && (!ty || trackTypeCounts(t)[ty] > 0)).length;
  const total = howMany(lang, level, type);
  const filtered = !!(lang || level || type);
  const chip = (active: boolean, n: number) => `inline-flex min-h-11 shrink-0 items-center gap-2 rounded-lg border px-3 py-2 text-sm ${active ? "border-accent bg-accent-soft font-medium text-text" : n === 0 ? "border-border text-muted opacity-60 hover:opacity-100" : "border-border text-text-2 hover:bg-surface-2 hover:text-text"}`;
  const chipLink = (key: string, to: string, active: boolean, n: number, children: React.ReactNode) => (
    <Link key={key} href={to} aria-current={active ? "true" : undefined} className={chip(active, n)} scroll={false}>
      {children}<span className={`font-mono text-xs ${active ? "text-text-2" : "text-muted"}`}>{n}</span>
    </Link>
  );
  const group = (label: string, hint: string, children: React.ReactNode) => (
    <div role="group" aria-label={label}>
      <p className="text-xs font-semibold uppercase tracking-wide text-muted">{label}{hint && <span className="font-normal normal-case tracking-normal"> {hint}</span>}</p>
      <div className="-mx-1 mt-1.5 flex gap-2 overflow-x-auto px-1 pb-1 sm:flex-wrap">{children}</div>
    </div>
  );
  const extra = (level ? 1 : 0) + (type ? 1 : 0);
  return (
    <section aria-label="Filter courses" className="mt-6">
      {group("Language", "", <>
        {chipLink("all", href(null, level, type), !lang, howMany(null, level, type), "Any")}
        {LANG_FILTERS.map((l) => chipLink(l.id, href(l.id, level, type), lang === l.id, howMany(l.id, level, type), l.label))}
      </>)}
      {/* Level and content type are tucked away until asked for, so the courses stay on screen. Open when one is active. */}
      <details open={extra > 0} className="group mt-2">
        <summary className="flex min-h-11 cursor-pointer list-none items-center gap-2 text-sm text-accent [&::-webkit-details-marker]:hidden">
          <span className="underline underline-offset-2">More filters{extra ? ` (${extra})` : ""}</span>
          <span aria-hidden="true" className="transition-transform group-open:rotate-180">▾</span>
          <span className="text-muted">Level, lesson types</span>
        </summary>
        <div className="mt-2 grid gap-4 rounded-xl border border-border bg-surface p-4">
          {group("Level", "how far along you are", <>
            {chipLink("any", href(lang, null, type), !level, howMany(lang, null, type), "Any")}
            {LEVEL_FILTERS.map((l) => chipLink(l, href(lang, l, type), level === l, howMany(lang, l, type), l))}
          </>)}
          {/* Types with no lessons are hidden, so the row only grows as practice exercises are added. */}
          {(type || TYPE_FILTERS.some((t) => t !== "lesson" && counts[t] > 0)) && group("Course includes", "at least one of", <>
            {chipLink("all", href(lang, level, null), !type, howMany(lang, level, null), "Anything")}
            {TYPE_FILTERS.filter((t) => t !== "lesson" && (counts[t] > 0 || t === type)).map((t) => chipLink(t, href(lang, level, t), type === t, howMany(lang, level, t), TYPE_LABEL[t]))}
          </>)}
        </div>
      </details>
      <div className="flex flex-wrap items-center justify-between gap-2 text-sm">
        <p role="status"><span className="font-semibold">{total}</span> <span className="text-muted">of {tracks.length} courses{filtered ? " match" : ""}</span></p>
        {filtered && <Link href="/learn" scroll={false} className="inline-flex min-h-11 items-center text-accent underline underline-offset-2">Clear filters</Link>}
      </div>
      {total === 0 && <p role="status" className="text-sm text-muted">No course has all of those together. <Link href="/learn" className="text-accent underline underline-offset-2">Clear the filters</Link>.</p>}
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
  const { user, progress } = await progressFor();
  const tracks = getTracks();
  const next = recommendLearning(tracks, progress, undefined, { experience: user?.experience });
  const done = next ? next.track.lessons.filter((l) => progress.get(l.exercise.id)?.status === "passed").length : 0;
  const started = progress.size > 0;
  return (
    <section aria-label="Recommended for you" className="mt-2 rounded-2xl border border-accent bg-accent-soft p-6 sm:p-10">
      <p className="text-sm font-semibold text-accent">{started ? "Continue where you left off" : user ? "Recommended for you" : "Start here"}</p>
      {next ? <>
        <h2 className="display mt-2 text-3xl font-bold sm:text-4xl">{next.track.title}</h2>
        <p className="mt-2 text-lg text-text-2">Next lesson: {next.lesson.title}</p>
        <p className="mt-3 max-w-2xl text-text-2">{next.reason ?? next.track.summary}</p>
        {started && <div className="mt-4 max-w-md"><div className="flex justify-between text-sm text-text-2"><span>{next.track.title}</span><span className="tabular-nums">{done} / {next.track.lessons.length} lessons</span></div><ProgressBar value={done / Math.max(1, next.track.lessons.length)} label={`${next.track.title} progress`} /></div>}
        <div className="mt-6 flex flex-wrap items-center gap-x-6 gap-y-2">
          <Link className="inline-flex min-h-12 items-center rounded-lg bg-accent px-6 text-lg font-semibold text-accent-ink" href={`/learn/${next.track.slug}/${next.lesson.slug}`}>{started ? "Continue" : "Start learning"} →</Link>
          <Link className="text-sm text-accent underline underline-offset-2" href={`/learn/${next.track.slug}`}>About this course</Link>
          <a className="text-sm text-accent underline underline-offset-2" href="#all-courses">Browse all courses ↓</a>
        </div>
      </> : <>
        <h2 className="display mt-2 text-3xl font-bold">You have finished this path.</h2>
        <p className="mt-2 text-text-2">Choose another course below, or review what you have learned.</p>
        <a className="mt-4 inline-block text-accent underline underline-offset-2" href="#all-courses">Browse all courses ↓</a>
      </>}
      {!user && <p className="mt-5 text-sm text-text-2">New to programming? <Link className="text-accent underline underline-offset-2" href="/learn/first-steps">Programming from Zero</Link> needs no experience. <Link className="text-accent underline underline-offset-2" href="/signup">Create an account</Link> to get a recommendation that fits you.</p>}
    </section>
  );
}
