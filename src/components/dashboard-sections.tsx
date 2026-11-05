import Link from "next/link";
import { ACHIEVEMENTS } from "@/lib/achievements";
import type { DailyPick } from "@/lib/daily";
import { groupByPath } from "@/lib/curriculum/paths";
import type { Track } from "@/lib/curriculum/types";
import type { ProgressDoc } from "@/lib/models";
import type { Rhythm } from "@/lib/rhythm";
import { sealsFor } from "@/lib/rewards-context";
import { Seal, SealLegend } from "./seals";
import { ButtonLink } from "./ui";
import { MapControls } from "./map-controls";

/** The weekly goal as a small ring plus plain-text counts. The rest-week detail is visible text. */
export function RhythmCard({ rhythm }: { rhythm: Rhythm }) {
  if (rhythm.goal <= 0) {
    return (
      <section aria-labelledby="rhythm-h" className="panel h-full p-6">
        <h2 id="rhythm-h" className="display-sm text-lg font-bold">Weekly rhythm</h2>
        <p className="mt-2 text-sm text-muted">You have no weekly goal set. <Link href="/settings" className="text-accent underline underline-offset-2">Choose one</Link> if a little structure helps.</p>
      </section>
    );
  }
  const frac = Math.min(1, rhythm.thisWeek / rhythm.goal);
  const R = 30;
  const C = 2 * Math.PI * R;
  const left = rhythm.goal - rhythm.thisWeek;
  return (
    <section aria-labelledby="rhythm-h" className="panel h-full p-6">
      <h2 id="rhythm-h" className="display-sm text-lg font-bold">Weekly rhythm</h2>
      <div className="mt-4 flex items-center gap-5">
        <div className="relative size-[76px] shrink-0">
          <svg viewBox="0 0 76 76" className="size-full -rotate-90" role="img" aria-label={`${rhythm.thisWeek} of ${rhythm.goal} lessons this week`}>
            <circle cx="38" cy="38" r={R} fill="none" stroke="var(--track, var(--line-strong))" strokeWidth="7" />
            <circle cx="38" cy="38" r={R} fill="none" stroke="var(--accent)" strokeWidth="7" strokeLinecap="round" strokeDasharray={C} strokeDashoffset={C * (1 - frac)} />
          </svg>
        </div>
        <div className="min-w-0 text-sm">
          <p className="font-semibold tabular-nums">{rhythm.thisWeek} of {rhythm.goal} {rhythm.goal === 1 ? "lesson" : "lessons"} this week</p>
          <p className="mt-1 text-muted">{left <= 0 ? `Goal met this week${rhythm.thisWeek > rhythm.goal ? ` (${rhythm.thisWeek} lessons)` : ""}.` : `${left} more ${left === 1 ? "lesson" : "lessons"} to reach it.`}</p>
          <p className="mt-2"><span className="display text-2xl font-bold text-accent">{rhythm.current}</span> <span className="text-muted">week streak, {rhythm.current === 1 ? "week" : "weeks"} in a row</span></p>
          {rhythm.best > rhythm.current && <p className="text-muted">Best: {rhythm.best} {rhythm.best === 1 ? "week" : "weeks"}</p>}
        </div>
      </div>
      <p className="mt-4 text-sm text-muted">
        {rhythm.rest > 0
          ? `${rhythm.rest} rest ${rhythm.rest === 1 ? "week" : "weeks"} saved: one covers a missed week automatically.`
          : "A rest week is saved for every four weeks you meet your goal, and covers a missed week automatically."}
      </p>
    </section>
  );
}

export type DueReview = { href: string; title: string; trackTitle: string };

export function ReviewCard({ due }: { due: DueReview[] }) {
  if (due.length === 0) return null;
  return (
    <section aria-labelledby="review-h" className="panel flex h-full flex-col p-6">
      <h2 id="review-h" className="display-sm text-lg font-bold">Ready for review</h2>
      <p className="mt-2 text-sm text-muted">{due.length} {due.length === 1 ? "lesson is" : "lessons are"} ready for a second look. Revisit earlier lessons to strengthen recall. Reviews don&apos;t earn XP.</p>
      <p className="mt-3 font-medium">{due[0].title}</p>
      <p className="text-sm text-muted">{due[0].trackTitle}</p>
      <div className="mt-auto pt-4">
        <ButtonLink href={due[0].href} variant="secondary">Review lesson</ButtonLink>
      </div>
    </section>
  );
}

export type ContinueTarget = { href: string; title: string; trackTitle: string; updatedAt: Date };

/** The lesson you touched most recently without passing it. Hidden when it is already the headline lesson. */
export function ContinueCard({ target }: { target: ContinueTarget }) {
  const when = target.updatedAt.toLocaleDateString("en-GB", { day: "numeric", month: "short", timeZone: "UTC" });
  return (
    <section aria-labelledby="continue-h" className="panel flex h-full flex-col p-6">
      <h2 id="continue-h" className="display-sm text-lg font-bold">Continue where you left off</h2>
      <p className="mt-2 break-words font-medium">{target.title}</p>
      <p className="text-sm text-muted">{target.trackTitle} · last worked on {when}</p>
      <p className="mt-3 text-sm text-muted">Your saved code is waiting.</p>
      <div className="mt-auto pt-4">
        <ButtonLink href={target.href} variant="secondary">Resume lesson</ButtonLink>
      </div>
    </section>
  );
}

export function PickCard({ pick }: { pick: DailyPick }) {
  return (
    <section aria-labelledby="pick-h" className="panel flex h-full flex-col p-6">
      <h2 id="pick-h" className="display-sm text-lg font-bold">Today&apos;s pick</h2>
      <p className="mt-2 text-sm font-semibold text-accent">{pick.label}</p>
      <p className="mt-1 break-words font-medium">{pick.lesson.title}</p>
      <p className="text-sm text-muted">{pick.track.title}</p>
      <p className="mt-3 text-sm text-muted">Just a suggestion. Skip it freely.</p>
      <div className="mt-auto pt-4">
        <ButtonLink href={pick.href} variant="secondary">Open</ButtonLink>
      </div>
    </section>
  );
}

/** The Codex: every achievement, earned ones first. Locked ones show their condition so they can be aimed at. */
export function CodexPanel({ unlocked, collapsed = false }: { unlocked: Map<string, Date>; collapsed?: boolean }) {
  const earned = ACHIEVEMENTS.filter((a) => unlocked.has(a.id)).sort((a, b) => +unlocked.get(b.id)! - +unlocked.get(a.id)!);
  const locked = ACHIEVEMENTS.filter((a) => !unlocked.has(a.id));
  const body = (
    <>
      <div className="flex flex-wrap items-baseline justify-between gap-2">
        <h2 id="codex-h" className="display text-[21px] font-bold">Codex <span className="text-accent">{earned.length}</span><span className="text-base font-normal text-muted"> of {ACHIEVEMENTS.length}</span></h2>
        <p className="text-sm text-muted">Recognition only. Achievements never change your XP.</p>
      </div>
      {earned.length === 0 && (
        <p className="mt-4 rounded-lg border border-line bg-surface-2 p-4 text-sm text-text-2">
          Nothing earned yet, and that is fine. Pass your first lesson and the first entry lands here. Locked ones below show what to aim for.
        </p>
      )}
      <ul className="mt-5 grid gap-3 sm:grid-cols-2 lg:grid-cols-3">
        {earned.map((a) => (
          <li key={a.id} className="rounded-lg border border-accent/40 bg-accent-soft p-4">
            <p className="font-semibold">{a.name}</p>
            <p className="mt-0.5 text-sm text-text-2">{a.description}</p>
            <p className="mt-1.5 text-sm text-muted">{unlocked.get(a.id)!.toLocaleDateString("en-GB", { day: "numeric", month: "short", year: "numeric" })}</p>
          </li>
        ))}
        {locked.map((a) => (
          <li key={a.id} className="rounded-lg border border-dashed border-line-strong p-4">
            <p className="font-semibold text-muted">{a.secret ? "A hidden achievement" : a.name}</p>
            <p className="mt-0.5 text-sm text-muted">{a.secret ? "Keep going and you may find it." : a.description}</p>
          </li>
        ))}
      </ul>
    </>
  );
  if (collapsed) {
    return (
      <details id="codex" className="panel group scroll-mt-24 p-5 open:p-6 open:sm:p-8 md:open:col-span-2">
        <summary className="flex cursor-pointer flex-wrap items-baseline justify-between gap-x-4 gap-y-1">
          <span className="display text-[19px] font-bold">Codex <span className="text-base font-normal text-muted">0 of {ACHIEVEMENTS.length} earned</span></span>
          <span className="text-sm text-muted">Pass a lesson to unlock the first entry. <span className="text-accent group-open:hidden">Show all</span></span>
        </summary>
        <div className="mt-5">{body}</div>
      </details>
    );
  }
  return <section id="codex" aria-labelledby="codex-h" className="panel scroll-mt-24 p-6 sm:p-8">{body}</section>;
}

export type MonthXp = { label: string; xp: number };

/** Private, comparison-free numbers: where the XP came from, how often hints were needed, and the seals earned. */
export function StatsPanel({ months, seals, firstTry, hints, tracks, collapsed = false }: { collapsed?: boolean; months: MonthXp[]; seals: { passed: number; unaided: number; challenge: number }; firstTry: number; hints: { recent: number; earlier: number | null }; tracks: { title: string; done: number; total: number }[] }) {
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
  if (collapsed) {
    return (
      <details className="panel group p-5 open:p-6 open:sm:p-8 md:open:col-span-2">
        <summary className="flex cursor-pointer flex-wrap items-baseline justify-between gap-x-4 gap-y-1">
          <span className="display text-[19px] font-bold">Your numbers</span>
          <span className="text-sm text-muted">Nothing to chart yet. <span className="text-accent group-open:hidden">Show</span></span>
        </summary>
        <div className="mt-5">{body}</div>
      </details>
    );
  }
  return <section aria-labelledby="stats-h" className="panel p-6 sm:p-8">{body}</section>;
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
    <section aria-labelledby="map-h" className="panel p-6 sm:p-8">
      <div className="flex flex-wrap items-baseline justify-between gap-3">
        <h2 id="map-h" className="display text-[21px] font-bold">Course map</h2>
        <div className="flex flex-wrap items-center gap-x-4">
          <SealLegend />
          <MapControls targetId="map-body" />
        </div>
      </div>
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
