"use client";

import Link from "next/link";
import { useRouter } from "next/navigation";
import { useCallback, useEffect, useRef, useState } from "react";

export type Reward = { xp: number; totalXp: number; level: number; title: string; blurb: string; levelUp: boolean };
export type Unlocked = { id: string; name: string; description: string };
export type TrackComplete = { slug: string; title: string; lessons: number; unaided: number; challenges: number; next: { slug: string; title: string } | null };

/** A short, quiet thank-you for each new achievement. Several unlocks stack; each fades on its own. */
export function AchievementToasts({ items, onDismiss }: { items: Unlocked[]; onDismiss: (id: string) => void }) {
  return (
    <div role="status" aria-live="polite" aria-atomic="false" aria-relevant="additions" className="pointer-events-none fixed inset-x-4 bottom-4 z-40 flex flex-col items-end gap-2 sm:left-auto sm:right-4">
      {items.map((a) => (
        <Toast key={a.id} item={a} onDismiss={onDismiss} />
      ))}
    </div>
  );
}

function Toast({ item, onDismiss }: { item: Unlocked; onDismiss: (id: string) => void }) {
  // The timer pauses while the pointer or keyboard focus is on the toast, so it can be read and its link reached.
  const [paused, setPaused] = useState(false);
  useEffect(() => {
    if (paused) return;
    const t = setTimeout(() => onDismiss(item.id), 8000);
    return () => clearTimeout(t);
  }, [item.id, onDismiss, paused]);
  return (
    <div
      onMouseEnter={() => setPaused(true)}
      onMouseLeave={() => setPaused(false)}
      onFocus={() => setPaused(true)}
      onBlur={() => setPaused(false)}
      className="toast-in panel pointer-events-auto flex w-full max-w-sm items-start gap-3 p-4 shadow-lg">
      <div className="min-w-0 flex-1">
        <p className="text-xs font-semibold text-accent">Achievement unlocked</p>
        <p className="display-sm mt-0.5 font-bold">{item.name}</p>
        <p className="mt-0.5 text-sm text-muted">{item.description}</p>
        <Link href="/dashboard#codex" className="mt-1 inline-block text-xs text-accent underline underline-offset-2">
          See your Codex
        </Link>
      </div>
      <button type="button" aria-label={`Dismiss ${item.name}`} onClick={() => onDismiss(item.id)} className="-m-2 flex size-11 items-center justify-center rounded-md text-muted hover:text-text">
        <span aria-hidden="true">×</span>
      </button>
    </div>
  );
}

export type NextLesson = { href: string; title: string };
type CelebrationItem = { id: number; levelUp: Reward | null; trackComplete: TrackComplete | null; nextLesson: NextLesson | null };
type FocusReturn = HTMLElement | (() => HTMLElement | null) | null;

/**
 * Level-up and course-complete moments, queued: a new pass while one is open waits its turn instead of
 * replacing or stacking. `onClose` fires once, when the queue is empty.
 */
export function Celebration({ levelUp, trackComplete, nextLesson = null, onClose, returnFocus }: { levelUp: Reward | null; trackComplete: TrackComplete | null; nextLesson?: NextLesson | null; onClose: () => void; returnFocus?: FocusReturn }) {
  const [queue, setQueue] = useState<CelebrationItem[]>([{ id: 0, levelUp, trackComplete, nextLesson }]);
  const [seen, setSeen] = useState({ levelUp, trackComplete });
  // Props that change identity while open are a fresh celebration: append it (adjusting state during render).
  if (seen.levelUp !== levelUp || seen.trackComplete !== trackComplete) {
    setSeen({ levelUp, trackComplete });
    if (levelUp || trackComplete) setQueue((q) => [...q, { id: (q[q.length - 1]?.id ?? 0) + 1, levelUp, trackComplete, nextLesson }]);
  }
  const advance = useCallback(() => {
    if (queue.length > 1) setQueue((q) => q.slice(1));
    else onClose();
  }, [queue.length, onClose]);
  const current = queue[0];
  return <CelebrationDialog key={current.id} item={current} remaining={queue.length - 1} onClose={advance} returnFocus={returnFocus} />;
}

/** A real dialog: focus moves in, Tab stays inside, Escape or Enter/Space on the card closes, N goes to the next lesson. */
function CelebrationDialog({ item, remaining, onClose, returnFocus }: { item: CelebrationItem; remaining: number; onClose: () => void; returnFocus?: FocusReturn }) {
  const { levelUp, trackComplete, nextLesson } = item;
  const router = useRouter();
  const ref = useRef<HTMLDivElement>(null);
  const [page, setPage] = useState<"track" | "level">(trackComplete ? "track" : "level");
  const titleId = "celebration-title";
  const descId = "celebration-desc";
  // Confetti only for finishing a course or reaching the top level, and only if motion is welcome (see globals.css).
  const confetti = (page === "track" && !!trackComplete) || (page === "level" && levelUp?.level === 10);

  // Remember the trigger once, and give focus back to it when the dialog goes away.
  useEffect(() => {
    const previous = typeof returnFocus === "function" ? null : returnFocus ?? document.activeElement as HTMLElement | null;
    return () => {
      // A pass can replace the trigger with the next-lesson action. Resolve a
      // caller-provided fallback at dismissal, after that replacement commits.
      const target = typeof returnFocus === "function" ? returnFocus() : previous;
      if (target?.isConnected) target.focus();
    };
  }, [returnFocus]);

  // Move focus to the primary action whenever the page changes (and on open).
  useEffect(() => {
    const node = ref.current;
    (node?.querySelector<HTMLElement>("[data-autofocus]") ?? node)?.focus();
  }, [page]);

  // Escape closes; N opens the next lesson; Enter or Space on the card itself closes; Tab stays inside the dialog.
  useEffect(() => {
    const node = ref.current;
    const onKey = (e: KeyboardEvent) => {
      if (e.key === "Escape") {
        e.preventDefault();
        e.stopPropagation();
        return onClose();
      }
      if ((e.key === "n" || e.key === "N") && nextLesson && !e.ctrlKey && !e.metaKey && !e.altKey) {
        e.preventDefault();
        e.stopPropagation();
        onClose();
        return router.push(nextLesson.href);
      }
      if ((e.key === "Enter" || e.key === " ") && document.activeElement === node) {
        e.preventDefault();
        e.stopPropagation();
        return onClose();
      }
      if (e.key !== "Tab" || !node) return;
      const focusable = [...node.querySelectorAll<HTMLElement>("a[href], button:not([disabled])")];
      if (!focusable.length) return e.preventDefault();
      const first = focusable[0];
      const last = focusable[focusable.length - 1];
      const active = document.activeElement;
      if (!node.contains(active)) {
        e.preventDefault();
        first.focus();
      } else if (e.shiftKey && (active === first || active === node)) {
        e.preventDefault();
        last.focus();
      } else if (!e.shiftKey && active === last) {
        e.preventDefault();
        first.focus();
      }
    };
    document.addEventListener("keydown", onKey, true);
    return () => document.removeEventListener("keydown", onKey, true);
  }, [onClose, nextLesson, router]);

  const hasBoth = !!trackComplete && !!levelUp;
  return (
    <div className="fixed inset-0 z-50 flex items-center justify-center bg-bg/80 p-4 backdrop-blur-sm" onMouseDown={(e) => e.target === e.currentTarget && onClose()}>
      <div ref={ref} role="dialog" aria-modal="true" aria-labelledby={titleId} aria-describedby={descId} tabIndex={-1} className="celebrate-in outline-none panel relative w-full max-w-md overflow-hidden p-7 text-center">
        {confetti && <Confetti />}
        {page === "track" && trackComplete ? (
          <>
            <p className="text-sm font-semibold text-accent">Course complete</p>
            <h2 id={titleId} className="display mt-2 text-3xl font-bold">
              {trackComplete.title}
            </h2>
            <p id={descId} className="mt-3 text-muted">
              You passed all {trackComplete.lessons} lessons.
              {trackComplete.unaided > 0 && <> {trackComplete.unaided} with an Unaided seal.</>}
              {trackComplete.challenges > 0 && <> {trackComplete.challenges} challenge{trackComplete.challenges === 1 ? "" : "s"} passed.</>}
            </p>
            <div className="mt-6 flex flex-col gap-2">
              {hasBoth ? (
                <button type="button" data-autofocus onClick={() => setPage("level")} className="h-11 rounded-lg bg-accent px-5 font-medium text-accent-ink hover:brightness-110">
                  Continue
                </button>
              ) : trackComplete.next ? (
                <Link data-autofocus href={`/learn/${trackComplete.next.slug}`} onClick={onClose} className="inline-flex h-11 items-center justify-center rounded-lg bg-accent px-5 font-medium text-accent-ink hover:brightness-110">
                  Next course: {trackComplete.next.title}
                </Link>
              ) : (
                <Link data-autofocus href="/dashboard" onClick={onClose} className="inline-flex h-11 items-center justify-center rounded-lg bg-accent px-5 font-medium text-accent-ink hover:brightness-110">
                  Back to your dashboard
                </Link>
              )}
              {!hasBoth && (
                <button type="button" onClick={onClose} className="h-11 rounded-lg px-5 text-sm text-muted hover:bg-surface-2 hover:text-text">
                  Stay here
                </button>
              )}
            </div>
          </>
        ) : (
          levelUp && (
            <>
              <p className="text-sm font-semibold text-accent">Level {levelUp.level}</p>
              <h2 id={titleId} className="display mt-2 text-4xl font-bold">
                {levelUp.title}
              </h2>
              <p id={descId} className="mt-3 text-muted">{levelUp.blurb}</p>
              <p className="mt-2 text-sm text-muted">{levelUp.totalXp} XP in total</p>
              <div className="mt-6 flex flex-col gap-2">
                {trackComplete?.next && page === "level" && hasBoth ? (
                  <Link data-autofocus href={`/learn/${trackComplete.next.slug}`} onClick={onClose} className="inline-flex h-11 items-center justify-center rounded-lg bg-accent px-5 font-medium text-accent-ink hover:brightness-110">
                    Next course: {trackComplete.next.title}
                  </Link>
                ) : nextLesson ? (
                  <Link data-autofocus href={nextLesson.href} onClick={onClose} className="inline-flex h-11 items-center justify-center rounded-lg bg-accent px-5 font-medium text-accent-ink hover:brightness-110">
                    Next lesson: {nextLesson.title}
                  </Link>
                ) : (
                  <button type="button" data-autofocus onClick={onClose} className="h-11 rounded-lg bg-accent px-5 font-medium text-accent-ink hover:brightness-110">
                    Keep going
                  </button>
                )}
                {((trackComplete?.next && hasBoth) || nextLesson) && (
                  <button type="button" onClick={onClose} className="h-11 rounded-lg px-5 text-sm text-muted hover:bg-surface-2 hover:text-text">
                    Stay here
                  </button>
                )}
              </div>
            </>
          )
        )}
        {remaining > 0 && <p className="mt-3 text-xs text-muted">{remaining} more after this</p>}
        {nextLesson && <p className="mt-3 text-xs text-muted">Press N for the next lesson, Esc to close.</p>}
      </div>
    </div>
  );
}

const CONFETTI = Array.from({ length: 26 }, (_, i) => ({ left: (i * 37) % 100, delay: (i % 7) * 0.12, hue: i % 3 }));

function Confetti() {
  return (
    <div aria-hidden="true" className="confetti pointer-events-none absolute inset-0 overflow-hidden">
      {CONFETTI.map((c, i) => (
        <span key={i} style={{ left: `${c.left}%`, animationDelay: `${c.delay}s` }} className={c.hue === 0 ? "bg-accent" : c.hue === 1 ? "bg-ok" : "bg-text-2"} />
      ))}
    </div>
  );
}
