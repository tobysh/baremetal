import Link from "next/link";
import { ACHIEVEMENTS } from "@/lib/achievements";
import type { DailyPick } from "@/lib/daily";
import { groupByPath } from "@/lib/curriculum/paths";
import type { Track } from "@/lib/curriculum/types";
import type { ProgressDoc } from "@/lib/models";
import type { Rhythm } from "@/lib/rhythm";
import { sealsFor } from "@/lib/rewards-context";
import { Seal, SealLegend } from "./seals";

/** "2 of 3 this week" as a small ring plus the rhythm count. The rest-week detail lives in a tooltip only. */
export function RhythmCard({ rhythm }: { rhythm: Rhythm }) {
  if (rhythm.goal <= 0) {
    return (
      <section aria-labelledby="rhythm-h" className="panel p-6">
        <h2 id="rhythm-h" className="display-sm text-lg font-bold">Weekly rhythm</h2>
        <p className="mt-2 text-sm text-muted">You have no weekly goal set. <Link href="/settings" className="text-accent underline underline-offset-2">Choose one</Link> if a little structure helps.</p>
      </section>
    );
  }
  const frac = Math.min(1, rhythm.thisWeek / rhythm.goal);
  const R = 30;
  const C = 2 * Math.PI * R;
  return (
    <section aria-labelledby="rhythm-h" className="panel p-6">
      <h2 id="rhythm-h" className="display-sm text-lg font-bold">Weekly rhythm</h2>
      <div className="mt-4 flex items-center gap-5">
        <div className="relative size-[76px] shrink-0">
          <svg viewBox="0 0 76 76" className="size-full -rotate-90" role="img" aria-label={`${rhythm.thisWeek} of ${rhythm.goal} this week`}>
            <circle cx="38" cy="38" r={R} fill="none" stroke="var(--line)" strokeWidth="7" />
            <circle cx="38" cy="38" r={R} fill="none" stroke="var(--accent)" strokeWidth="7" strokeLinecap="round" strokeDasharray={C} strokeDashoffset={C * (1 - frac)} />
          </svg>
          <span className="absolute inset-0 flex items-center justify-center text-sm font-semibold tabular-nums">{rhythm.thisWeek}/{rhythm.goal}</span>
        </div>
        <div className="min-w-0 text-sm">
          <p><span className="display text-2xl font-bold text-accent">{rhythm.current}</span> <span className="text-muted" title={rhythm.rest > 0 ? `${rhythm.rest} rest week${rhythm.rest === 1 ? "" : "s"} banked: one covers a missed week automatically.` : "A rest week is banked for every four weeks you meet your goal."}>{rhythm.current === 1 ? "week" : "weeks"} in a row</span></p>
          <p className="mt-1 text-muted">{rhythm.thisWeek >= rhythm.goal ? "Goal met this week." : `${rhythm.goal - rhythm.thisWeek} more ${rhythm.goal - rhythm.thisWeek === 1 ? "lesson" : "lessons"} this week.`}</p>
          {rhythm.best > rhythm.current && <p className="text-xs text-muted">Best: {rhythm.best} weeks</p>}
        </div>
      </div>
    </section>
  );
}

export type DueReview = { href: string; title: string; trackTitle: string };

export function ReviewCard({ due }: { due: DueReview[] }) {
  if (due.length === 0) return null;
  return (
    <section aria-labelledby="review-h" className="panel p-6">
      <h2 id="review-h" className="display-sm text-lg font-bold">Ready for review</h2>
      <p className="mt-2 text-sm text-muted">{due.length} {due.length === 1 ? "lesson is" : "lessons are"} ready for a second look. Reviews give no XP; they keep what you learned fresh.</p>
      <Link href={due[0].href} className="mt-4 inline-flex min-h-10 items-center rounded-lg border border-line-strong px-4 text-sm font-medium hover:bg-surface-2">
        Review {due[0].title}
      </Link>
    </section>
  );
}

export function PickCard({ pick }: { pick: DailyPick }) {
  return (
    <section aria-labelledby="pick-h" className="panel p-6">
      <h2 id="pick-h" className="display-sm text-lg font-bold">Today&apos;s pick</h2>
      <p className="mt-2 text-xs font-semibold text-accent">{pick.label}</p>
      <p className="mt-1 font-medium">{pick.lesson.title}</p>
      <p className="text-sm text-muted">{pick.track.title}</p>
      <Link href={pick.href} className="mt-4 inline-flex min-h-10 items-center rounded-lg border border-line-strong px-4 text-sm font-medium hover:bg-surface-2">
        Open
      </Link>
      <p className="mt-3 text-xs text-muted">Just a suggestion. Skip it freely.</p>
    </section>
  );
}

/** The Codex: every achievement, earned ones first. Locked ones show their condition so they can be aimed at. */
export function CodexPanel({ unlocked }: { unlocked: Map<string, Date> }) {
  const earned = ACHIEVEMENTS.filter((a) => unlocked.has(a.id)).sort((a, b) => +unlocked.get(b.id)! - +unlocked.get(a.id)!);
  const locked = ACHIEVEMENTS.filter((a) => !unlocked.has(a.id));
  return (
    <section id="codex" aria-labelledby="codex-h" className="panel scroll-mt-24 p-6 sm:p-8">
      <div className="flex flex-wrap items-baseline justify-between gap-2">
        <h2 id="codex-h" className="display text-[21px] font-bold">Codex <span className="text-accent">{earned.length}</span><span className="text-base font-normal text-muted"> of {ACHIEVEMENTS.length}</span></h2>
        <p className="text-sm text-muted">Recognition only. Achievements never change your XP.</p>
      </div>
      <ul className="mt-5 grid gap-3 sm:grid-cols-2 lg:grid-cols-3">
        {earned.map((a) => (
          <li key={a.id} className="rounded-lg border border-accent/40 bg-accent-soft p-4">
            <p className="font-semibold">{a.name}</p>
            <p className="mt-0.5 text-sm text-text-2">{a.description}</p>
            <p className="mt-1.5 text-xs text-muted">{unlocked.get(a.id)!.toLocaleDateString("en-GB", { day: "numeric", month: "short", year: "numeric" })}</p>
          </li>
        ))}
        {locked.map((a) => (
          <li key={a.id} className="rounded-lg border border-dashed border-line-strong p-4">
            <p className="font-semibold text-muted">{a.secret ? "A hidden achievement" : a.name}</p>
            <p className="mt-0.5 text-sm text-muted">{a.secret ? "Keep going and you may find it." : a.description}</p>
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
  return (
    <section aria-labelledby="stats-h" className="panel p-6 sm:p-8">
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
              {hints.earlier !== null && <dd className="text-xs text-muted">{hints.earlier.toFixed(1)} before that</dd>}
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
    </section>
  );
}

/** Each course as a path of lesson nodes; courses that build on another are joined to it. Phones get a plain list. */
export function CourseMap({ tracks, progress }: { tracks: Track[]; progress: Map<string, ProgressDoc> }) {
  const groups = groupByPath(tracks);
  const done = (t: Track) => t.lessons.filter((l) => progress.get(l.exercise.id)?.status === "passed").length;
  const complete = (t: Track) => t.lessons.length > 0 && done(t) === t.lessons.length;
  const bySlug = new Map(tracks.map((t) => [t.slug, t]));
  return (
    <section aria-labelledby="map-h" className="panel p-6 sm:p-8">
      <div className="flex flex-wrap items-baseline justify-between gap-3">
        <h2 id="map-h" className="display text-[21px] font-bold">Course map</h2>
        <SealLegend />
      </div>
      {/* Phones: one line per course. */}
      <ul className="mt-5 space-y-3 sm:hidden">
        {tracks.map((t) => (
          <li key={t.slug}>
            <Link href={`/learn/${t.slug}`} className="flex items-baseline justify-between gap-3 rounded-lg border border-line px-4 py-3 hover:bg-surface-2">
              <span className="min-w-0 font-medium">{t.title}</span>
              <span className="shrink-0 text-sm text-muted tabular-nums">{done(t)}/{t.lessons.length}</span>
            </Link>
          </li>
        ))}
      </ul>
      <div className="mt-6 hidden space-y-8 sm:block">
        {groups.map((g) => (
          <div key={g.id}>
            <h3 className="text-xs font-semibold uppercase tracking-wide text-muted">{g.title}</h3>
            <ol className="mt-3 space-y-4">
              {g.tracks.map((t) => {
                const pre = t.prerequisite ? bySlug.get(t.prerequisite) : undefined;
                return (
                  <li key={t.slug}>
                    {pre && (
                      <p className="mb-1 flex items-center gap-2 text-xs text-muted">
                        <span aria-hidden="true" className="h-4 w-px bg-line-strong" />
                        {complete(pre) ? "Builds on" : "Best after"} {pre.title}
                      </p>
                    )}
                    <div className="flex flex-wrap items-center gap-x-4 gap-y-2">
                      <Link href={`/learn/${t.slug}`} className="w-56 shrink-0 text-sm font-medium hover:text-accent">
                        {t.title} <span className="font-normal text-muted">{done(t)}/{t.lessons.length}</span>
                      </Link>
                      <ol className="flex min-w-0 flex-wrap items-center gap-1.5" aria-label={`${t.title} lessons`}>
                        {t.lessons.map((l, i) => {
                          const s = sealsFor(progress, l.exercise.id);
                          const state = s.passed ? "passed" : "not passed";
                          return (
                            <li key={l.slug}>
                              <Link
                                href={`/learn/${t.slug}/${l.slug}`}
                                aria-label={`Lesson ${i + 1}: ${l.title}, ${state}${s.unaided ? ", unaided" : ""}${s.challenge ? ", challenge passed" : ""}`}
                                title={l.title}
                                className={`relative flex size-7 items-center justify-center rounded-full border-2 text-[11px] font-semibold hover:border-accent ${s.passed ? "border-accent bg-accent text-accent-ink" : "border-line-strong text-muted"}`}
                              >
                                {s.passed ? <svg width="12" height="12" viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="3.4" strokeLinecap="round" strokeLinejoin="round" aria-hidden="true"><polyline points="4 12.5 9.5 18 20 6.5" /></svg> : i + 1}
                                {(s.unaided || s.challenge) && (
                                  <span aria-hidden="true" className="absolute -right-1 -top-1 flex gap-px rounded-full bg-surface">
                                    {s.unaided && <Seal kind="unaided" earned size={11} />}
                                    {s.challenge && <Seal kind="challenge" earned size={11} />}
                                  </span>
                                )}
                              </Link>
                            </li>
                          );
                        })}
                      </ol>
                    </div>
                  </li>
                );
              })}
            </ol>
          </div>
        ))}
      </div>
    </section>
  );
}
