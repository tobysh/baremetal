import Link from "next/link";
import { ACHIEVEMENTS } from "@/lib/achievements";
import type { DailyPick } from "@/lib/daily";
import { groupByPath } from "@/lib/curriculum/paths";
import type { Track } from "@/lib/curriculum/types";
import type { ProgressDoc } from "@/lib/models";
import type { Rhythm } from "@/lib/rhythm";
import { sealsFor } from "@/lib/rewards-context";
import { Seal, SealLegend } from "./seals";
import { ButtonLink, ProgressBar } from "./ui";
import { MapControls } from "./map-controls";

/**
 * The "today" strip: review, resume and weekly rhythm are segments of ONE panel, divided by hairlines.
 * Segments size to their content (no h-full), so mismatched heights never leave a hole in the grid.
 */
export function ActivityStrip({ children }: { children: React.ReactNode }) {
  return <div className="panel grid divide-y divide-line md:grid-cols-[repeat(auto-fit,minmax(260px,1fr))] md:divide-x md:divide-y-0">{children}</div>;
}

const segment = "flex min-w-0 flex-col gap-1 p-4 sm:p-5";

/** The weekly goal as a small ring plus plain-text counts. The rest-week detail is visible text. */
export function RhythmCard({ rhythm }: { rhythm: Rhythm }) {
  if (rhythm.goal <= 0) {
    return (
      <section aria-labelledby="rhythm-h" className={segment}>
        <h2 id="rhythm-h" className="display-sm text-base font-bold">Weekly rhythm</h2>
        <p className="text-sm text-muted">You have no weekly goal set. <Link href="/settings" className="text-accent underline underline-offset-2">Choose one</Link> if a little structure helps.</p>
      </section>
    );
  }
  const frac = Math.min(1, rhythm.thisWeek / rhythm.goal);
  const R = 24;
  const C = 2 * Math.PI * R;
  const left = rhythm.goal - rhythm.thisWeek;
  return (
    <section aria-labelledby="rhythm-h" className={segment}>
      <h2 id="rhythm-h" className="display-sm text-base font-bold">Weekly rhythm</h2>
      <div className="mt-1 flex items-center gap-4">
        <div className="relative size-14 shrink-0">
          <svg viewBox="0 0 60 60" className="size-full -rotate-90" role="img" aria-label={`${rhythm.thisWeek} of ${rhythm.goal} lessons this week`}>
            <circle cx="30" cy="30" r={R} fill="none" stroke="var(--track, var(--line-strong))" strokeWidth="6" />
            <circle cx="30" cy="30" r={R} fill="none" stroke="var(--accent)" strokeWidth="6" strokeLinecap="round" strokeDasharray={C} strokeDashoffset={C * (1 - frac)} />
          </svg>
        </div>
        <div className="min-w-0 text-sm">
          <p className="font-semibold tabular-nums">{rhythm.thisWeek} of {rhythm.goal} {rhythm.goal === 1 ? "lesson" : "lessons"} this week{left <= 0 && ". Goal met"}</p>
          <p className="text-muted"><span className="font-semibold text-text tabular-nums">{rhythm.current}</span> week streak{rhythm.best > rhythm.current && ` · best ${rhythm.best}`}</p>
        </div>
      </div>
      <details className="mt-1 text-sm text-muted">
        <summary className="inline-flex min-h-9 cursor-pointer items-center font-medium text-text-2 hover:text-accent">How rest weeks work</summary>
        <p className="mt-1">
          {rhythm.rest > 0
            ? `${rhythm.rest} rest ${rhythm.rest === 1 ? "week" : "weeks"} saved: one covers a missed week automatically. `
            : ""}
          A rest week is saved for every four weeks you meet your goal, and covers a missed week automatically.
        </p>
      </details>
    </section>
  );
}

export type DueReview = { href: string; title: string; trackTitle: string };

export function ReviewCard({ due }: { due: DueReview[] }) {
  if (due.length === 0) return null;
  const more = due.length - 1;
  return (
    <section aria-labelledby="review-h" className={segment}>
      <h2 id="review-h" className="display-sm text-base font-bold">Ready for review</h2>
      <p className="text-sm text-muted"><span className="font-semibold text-text tabular-nums">{due.length}</span> {due.length === 1 ? "lesson is" : "lessons are"} due{more > 0 && ` · up next: ${due[0].title}`}</p>
      {more === 0 && <p className="break-words text-sm font-medium">{due[0].title} <span className="font-normal text-muted">· {due[0].trackTitle}</span></p>}
      <div className="pt-2"><ButtonLink href={due[0].href} variant="secondary">Start review</ButtonLink></div>
    </section>
  );
}

export type ContinueTarget = { href: string; title: string; trackTitle: string; updatedAt: Date };

/** The lesson you touched most recently without passing it. Hidden when it is already the headline lesson. */
export function ContinueCard({ target }: { target: ContinueTarget }) {
  const when = target.updatedAt.toLocaleDateString("en-GB", { day: "numeric", month: "short", timeZone: "UTC" });
  return (
    <section aria-labelledby="continue-h" className={segment}>
      <h2 id="continue-h" className="display-sm text-base font-bold">Continue where you left off</h2>
      <p className="break-words text-sm font-medium">{target.title}</p>
      <p className="text-sm text-muted">{target.trackTitle} · last worked on {when}. Your saved code is waiting.</p>
      <div className="pt-2"><ButtonLink href={target.href} variant="secondary">Resume lesson</ButtonLink></div>
    </section>
  );
}

/** Desktop only: on phones the strip keeps to review, resume and rhythm. */
export function PickCard({ pick }: { pick: DailyPick }) {
  return (
    <section aria-labelledby="pick-h" className={`${segment} hidden md:flex`}>
      <h2 id="pick-h" className="display-sm text-base font-bold">Today&apos;s pick</h2>
      <p className="text-sm font-semibold text-text-2">{pick.label}</p>
      <p className="break-words text-sm font-medium">{pick.lesson.title} <span className="font-normal text-muted">· {pick.track.title}</span></p>
      <p className="text-sm text-muted">Just a suggestion. Skip it freely.</p>
      <div className="pt-2"><ButtonLink href={pick.href} variant="secondary">Open</ButtonLink></div>
    </section>
  );
}

export type CodexFilter = "earned" | "all" | "locked";

const fmtDate = (d: Date) => d.toLocaleDateString("en-GB", { day: "numeric", month: "short", year: "numeric" });

/** Dashboard preview: the most recent entries only. The full list lives at /dashboard/codex. */
export function CodexPreview({ unlocked }: { unlocked: Map<string, Date> }) {
  const earned = ACHIEVEMENTS.filter((a) => unlocked.has(a.id)).sort((a, b) => +unlocked.get(b.id)! - +unlocked.get(a.id)!);
  return (
    <section aria-labelledby="codex-h" className="panel flex min-w-0 flex-col p-4 sm:p-5">
      <div className="flex items-baseline justify-between gap-3">
        <h2 id="codex-h" className="display-sm text-base font-bold">Codex <span className="font-normal text-muted tabular-nums">{earned.length} of {ACHIEVEMENTS.length}</span></h2>
        <Link href="/dashboard/codex" className="inline-flex min-h-9 items-center text-sm font-medium text-accent hover:underline">View Codex</Link>
      </div>
      {earned.length === 0 ? (
        <p className="mt-2 text-sm text-muted">Nothing earned yet, and that is fine. Pass your first lesson and the first entry lands here.</p>
      ) : (
        <ul className="mt-2 divide-y divide-line">
          {earned.slice(0, 3).map((a) => (
            <li key={a.id} className="py-2">
              <p className="text-sm font-semibold">{a.name} <span className="font-normal text-muted">· {fmtDate(unlocked.get(a.id)!)}</span></p>
              <p className="text-sm text-text-2">{a.description}</p>
            </li>
          ))}
        </ul>
      )}
    </section>
  );
}

/** The full Codex with Earned / All / Locked views. Locked entries show their condition so they can be aimed at. */
export function CodexPanel({ unlocked, filter }: { unlocked: Map<string, Date>; filter: CodexFilter }) {
  const earned = ACHIEVEMENTS.filter((a) => unlocked.has(a.id)).sort((a, b) => +unlocked.get(b.id)! - +unlocked.get(a.id)!);
  const locked = ACHIEVEMENTS.filter((a) => !unlocked.has(a.id));
  const tabs: { id: CodexFilter; label: string; count: number }[] = [
    { id: "earned", label: "Earned", count: earned.length },
    { id: "all", label: "All", count: ACHIEVEMENTS.length },
    { id: "locked", label: "Locked", count: locked.length },
  ];
  const showEarned = filter !== "locked";
  const showLocked = filter !== "earned";
  return (
    <section aria-labelledby="codex-h">
      <p className="text-sm text-muted">Recognition only. Achievements never change your XP.</p>
      <h2 id="codex-h" className="sr-only">Achievements</h2>
      <nav aria-label="Codex view" className="mt-4 flex gap-2 overflow-x-auto">
        {tabs.map((t) => (
          <Link key={t.id} href={`/dashboard/codex?show=${t.id}`} replace aria-current={t.id === filter ? "page" : undefined} className={`inline-flex min-h-11 shrink-0 items-center gap-1.5 rounded-full border px-4 text-sm font-medium ${t.id === filter ? "border-accent bg-accent-soft text-accent" : "border-line-strong text-text-2 hover:bg-surface-2"}`}>
            {t.label} <span className="tabular-nums text-muted">{t.count}</span>
          </Link>
        ))}
      </nav>
      {filter === "earned" && earned.length === 0 && (
        <p className="mt-4 rounded-lg border border-line bg-surface-2 p-4 text-sm text-text-2">
          Nothing earned yet, and that is fine. Pass your first lesson and the first entry lands here. <Link href="/dashboard/codex?show=locked" className="text-accent underline underline-offset-2">See what to aim for</Link>.
        </p>
      )}
      <ul className="mt-4 divide-y divide-line rounded-lg border border-line">
        {showEarned && earned.map((a) => (
          <li key={a.id} className="bg-accent-soft/40 px-4 py-3">
            <div className="flex flex-wrap items-baseline justify-between gap-x-4">
              <p className="font-semibold">{a.name}</p>
              <p className="text-sm text-muted">{fmtDate(unlocked.get(a.id)!)}</p>
            </div>
            <p className="text-sm text-text-2">{a.description}</p>
          </li>
        ))}
        {showLocked && locked.map((a) => (
          <li key={a.id} className="px-4 py-3">
            <p className="font-semibold text-text-2">{a.secret ? "A hidden achievement" : a.name}</p>
            <p className="text-sm text-muted">{a.secret ? "Keep going and you may find it." : a.description}</p>
          </li>
        ))}
      </ul>
    </section>
  );
}

export type MonthXp = { label: string; xp: number };

/** Private, comparison-free numbers: where the XP came from, how often hints were needed, and the seals earned. */
export function StatsPanel({ months, seals, firstTry, hints, tracks }: { months: MonthXp[]; seals: { passed: number; unaided: number; challenge: number }; firstTry: number; hints: { recent: number; earlier: number | null }; tracks: { title: string; done: number; total: number }[] }) {
  const max = Math.max(1, ...months.map((m) => m.xp));
  const W = 300;
  const H = 110;
  const bw = W / months.length;
  const body = (
    <>
      <h2 id="stats-h" className="display text-[21px] font-bold">Your numbers</h2>
      <p className="mt-1 text-sm text-muted">Only you can see these.</p>
      <div className="mt-6 grid gap-8 lg:grid-cols-2">
        <div>
          <h3 className="text-sm font-semibold">XP by month</h3>
          <svg viewBox={`0 0 ${W} ${H + 22}`} role="img" aria-label={`XP by month: ${months.map((m) => `${m.label} ${m.xp}`).join(", ")}`} className="mt-3 w-full max-w-md">
            {months.map((m, i) => {
              const h = Math.round((m.xp / max) * H);
              return (
                <g key={m.label}>
                  <rect x={i * bw + 8} y={H - h} width={bw - 16} height={Math.max(h, m.xp > 0 ? 2 : 0)} rx="3" fill="var(--accent)" />
                  <text x={i * bw + bw / 2} y={H + 15} textAnchor="middle" fontSize="10" fill="var(--muted)">{m.label}</text>
                  {m.xp > 0 && <text x={i * bw + bw / 2} y={Math.max(10, H - h - 4)} textAnchor="middle" fontSize="10" fill="var(--text)">{m.xp}</text>}
                </g>
              );
            })}
            <line x1="0" x2={W} y1={H} y2={H} stroke="var(--line-strong)" />
          </svg>
        </div>
        <div className="space-y-5">
          <div>
            <h3 className="text-sm font-semibold">Seals earned</h3>
            <ul className="mt-2 flex flex-wrap gap-x-6 gap-y-2 text-sm">
              <li className="inline-flex items-center gap-2"><Seal kind="passed" earned size={20} /> {seals.passed} passed</li>
              <li className="inline-flex items-center gap-2"><Seal kind="unaided" earned size={20} /> {seals.unaided} unaided</li>
              <li className="inline-flex items-center gap-2"><Seal kind="challenge" earned size={20} /> {seals.challenge} challenges</li>
            </ul>
          </div>
          <dl className="grid grid-cols-2 gap-4 text-sm">
            <div><dt className="text-muted">Passed on the first run</dt><dd className="display text-2xl font-bold">{firstTry}</dd></div>
            <div>
              <dt className="text-muted">Hints per lesson, lately</dt>
              <dd className="display text-2xl font-bold">{hints.recent.toFixed(1)}</dd>
              {hints.earlier !== null && <dd className="text-sm text-muted">{hints.earlier.toFixed(1)} before that</dd>}
            </div>
          </dl>
          <div>
            <h3 className="text-sm font-semibold">By course</h3>
            <ul className="mt-2 space-y-1 text-sm">
              {tracks.filter((t) => t.done > 0).map((t) => (
                <li key={t.title} className="flex justify-between gap-4"><span className="min-w-0 truncate">{t.title}</span><span className="text-muted tabular-nums">{t.done}/{t.total}</span></li>
              ))}
              {tracks.every((t) => t.done === 0) && <li className="text-muted">Pass a lesson to start filling this in.</li>}
            </ul>
          </div>
        </div>
      </div>
    </>
  );
  return <section aria-labelledby="stats-h" className="panel p-5 sm:p-8">{body}</section>;
}

/** Lesson nodes for one course. Each circle has a 36px hit area even though it draws at 28px. */
function LessonDots({ t, progress, upNext }: { t: Track; progress: Map<string, ProgressDoc>; upNext?: string }) {
  return (
    <ol className="flex min-w-0 flex-wrap items-center gap-1.5" aria-label={`${t.title} lessons`}>
      {t.lessons.map((l, i) => {
        const s = sealsFor(progress, l.exercise.id);
        const isNext = l.slug === upNext && !s.passed;
        const state = s.passed ? "passed" : progress.has(l.exercise.id) ? "in progress" : "not started";
        return (
          <li key={l.slug}>
            <Link
              href={`/learn/${t.slug}/${l.slug}`}
              aria-current={isNext ? "step" : undefined}
              aria-label={`Lesson ${i + 1}: ${l.title}, ${isNext ? "up next, " : ""}${state}${s.unaided ? ", unaided" : ""}${s.challenge ? ", challenge passed" : ""}`}
              title={l.title}
              className={`relative flex size-7 items-center justify-center rounded-full border-2 text-xs font-semibold before:absolute before:-inset-1 before:content-[''] hover:border-accent ${isNext ? "outline outline-2 outline-offset-2 outline-accent" : ""} ${s.passed ? "border-accent bg-accent text-accent-ink" : "border-line-strong text-muted"}`}
            >
              {s.passed ? <svg width="12" height="12" viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="3.4" strokeLinecap="round" strokeLinejoin="round" aria-hidden="true"><polyline points="4 12.5 9.5 18 20 6.5" /></svg> : i + 1}
              {(s.unaided || s.challenge) && (
                <span aria-hidden="true" className="absolute -right-1.5 -top-1.5 flex gap-px rounded-full bg-surface">
                  {s.unaided && <Seal kind="unaided" earned size={14} />}
                  {s.challenge && <Seal kind="challenge" earned size={14} />}
                </span>
              )}
            </Link>
          </li>
        );
      })}
    </ol>
  );
}

/**
 * Courses grouped by path. Groups and courses are native disclosures: courses under way and the recommended
 * one start open, everything else is a single line. Phones get a plain list per group.
 */
export function CourseMap({ tracks, progress, recommendedSlug, upNextLessonSlug }: { tracks: Track[]; progress: Map<string, ProgressDoc>; recommendedSlug?: string; upNextLessonSlug?: string }) {
  const groups = groupByPath(tracks);
  const done = (t: Track) => t.lessons.filter((l) => progress.get(l.exercise.id)?.status === "passed").length;
  const started = (t: Track) => t.lessons.some((l) => progress.has(l.exercise.id));
  const complete = (t: Track) => t.lessons.length > 0 && done(t) === t.lessons.length;
  const isOpen = (t: Track) => t.slug === recommendedSlug || (started(t) && !complete(t));
  const bySlug = new Map(tracks.map((t) => [t.slug, t]));
  return (
    <section aria-labelledby="map-h" className="panel hidden p-6 sm:p-8 md:block">
      <div className="flex flex-wrap items-baseline justify-between gap-3">
        <h2 id="map-h" className="display text-[21px] font-bold">Course map</h2>
        <MapControls targetId="map-body" />
      </div>
      <div className="mt-2"><SealLegend /></div>
      <div id="map-body" className="mt-4 divide-y divide-line">
        {groups.map((g) => {
          const groupDone = g.tracks.reduce((n, t) => n + done(t), 0);
          const groupTotal = g.tracks.reduce((n, t) => n + t.lessons.length, 0);
          return (
            <details key={g.id} open={g.tracks.some(isOpen)} className="group/g py-3">
              <summary className="flex min-h-9 cursor-pointer flex-wrap items-baseline justify-between gap-x-4 gap-y-0.5">
                <span className="text-sm font-semibold">{g.title} <span className="font-normal text-muted">{g.tracks.length} {g.tracks.length === 1 ? "course" : "courses"}</span></span>
                <span className="text-sm text-muted tabular-nums">{groupDone}/{groupTotal} lessons</span>
              </summary>
              {/* Phones: one line per course. */}
              <ul className="mt-3 space-y-2 sm:hidden">
                {g.tracks.map((t) => (
                  <li key={t.slug}>
                    <Link href={`/learn/${t.slug}`} className="flex items-baseline justify-between gap-3 rounded-lg border border-line px-4 py-3 hover:bg-surface-2">
                      <span className="min-w-0 font-medium">{t.title}</span>
                      <span className="shrink-0 text-sm text-muted tabular-nums">{done(t)}/{t.lessons.length}</span>
                    </Link>
                  </li>
                ))}
              </ul>
              <ol className="mt-3 hidden space-y-1 sm:block">
                {g.tracks.map((t) => {
                  const pre = t.prerequisite ? bySlug.get(t.prerequisite) : undefined;
                  const hint = pre ? `${complete(pre) ? "Builds on" : "Best after"} ${pre.title}` : undefined;
                  return (
                    <li key={t.slug}>
                      <details open={isOpen(t)} className="rounded-lg px-2 py-1 open:bg-surface-2/40">
                        <summary title={hint} className="flex min-h-9 cursor-pointer flex-wrap items-baseline gap-x-3 text-sm font-medium">
                          <span>{t.title}</span>
                          <span className="font-normal text-muted tabular-nums">{done(t)}/{t.lessons.length}</span>
                          {t.slug === recommendedSlug && hint && <span className="text-sm font-normal text-accent">{hint}</span>}
                        </summary>
                        <div className="pb-2 pt-1">
                          <LessonDots t={t} progress={progress} upNext={t.slug === recommendedSlug ? upNextLessonSlug : undefined} />
                          {t.slug === recommendedSlug && upNextLessonSlug && t.lessons.some((l) => l.slug === upNextLessonSlug) && <p className="mt-2 text-sm text-text-2">Ringed lesson: up next.</p>}
                          <Link href={`/learn/${t.slug}`} className="mt-2 inline-flex min-h-9 items-center text-sm text-muted hover:text-accent">Course page</Link>
                        </div>
                      </details>
                    </li>
                  );
                })}
              </ol>
            </details>
          );
        })}
      </div>
    </section>
  );
}

/** Phones: one course, its progress and the way to the full list. The full map is desktop; every course is also listed at /learn. */
export function CoursePreview({ track, done, nextTitle }: { track: Track; done: number; nextTitle: string }) {
  return (
    <section aria-labelledby="preview-h" className="panel p-4 md:hidden">
      <div className="flex items-baseline justify-between gap-3">
        <h2 id="preview-h" className="display-sm text-base font-bold">Course path</h2>
        <Link href="/learn" className="inline-flex min-h-9 items-center text-sm font-medium text-accent hover:underline">All courses</Link>
      </div>
      <Link href={`/learn/${track.slug}`} className="mt-2 block rounded-lg py-1">
        <p className="font-medium">{track.title}</p>
        <p className="text-sm text-muted tabular-nums">{done} of {track.lessons.length} lessons · next: {nextTitle}</p>
      </Link>
      <div className="mt-2"><ProgressBar value={done / Math.max(1, track.lessons.length)} label={`${track.title} progress`} /></div>
    </section>
  );
}
