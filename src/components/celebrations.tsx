"use client";

import Link from "next/link";
import { useEffect, useRef, useState } from "react";

export type Reward = { xp: number; totalXp: number; level: number; title: string; blurb: string; levelUp: boolean };
export type Unlocked = { id: string; name: string; description: string };
export type TrackComplete = { slug: string; title: string; lessons: number; unaided: number; challenges: number; next: { slug: string; title: string } | null };

/** A short, quiet thank-you for each new achievement. Several unlocks stack; each fades on its own. */
export function AchievementToasts({ items, onDismiss }: { items: Unlocked[]; onDismiss: (id: string) => void }) {
  return (
    <div role="status" aria-live="polite" className="pointer-events-none fixed inset-x-4 bottom-4 z-40 flex flex-col items-end gap-2 sm:left-auto sm:right-4">
      {items.map((a) => (
        <Toast key={a.id} item={a} onDismiss={onDismiss} />
      ))}
    </div>
  );
}

function Toast({ item, onDismiss }: { item: Unlocked; onDismiss: (id: string) => void }) {
  useEffect(() => {
    const t = setTimeout(() => onDismiss(item.id), 8000);
    return () => clearTimeout(t);
  }, [item.id, onDismiss]);
  return (
    <div className="toast-in panel pointer-events-auto flex w-full max-w-sm items-start gap-3 p-4 shadow-lg">
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

/** Level-up and course-complete moments. A real dialog: focus moves in, Tab stays inside, Escape closes. */
export function Celebration({ levelUp, trackComplete, onClose }: { levelUp: Reward | null; trackComplete: TrackComplete | null; onClose: () => void }) {
  const ref = useRef<HTMLDivElement>(null);
  const [page, setPage] = useState<"track" | "level">(trackComplete ? "track" : "level");
  const titleId = "celebration-title";
  // Confetti only for finishing a course or reaching the top level, and only if motion is welcome (see globals.css).
  const confetti = (page === "track" && !!trackComplete) || (page === "level" && levelUp?.level === 10);

  useEffect(() => {
    const previous = document.activeElement as HTMLElement | null;
    const node = ref.current;
    node?.querySelector<HTMLElement>("[data-autofocus]")?.focus();
    const onKey = (e: KeyboardEvent) => {
      if (e.key === "Escape") return onClose();
      if (e.key !== "Tab" || !node) return;
      const focusable = [...node.querySelectorAll<HTMLElement>("a[href], button:not([disabled])")];
      if (!focusable.length) return;
      const first = focusable[0];
      const last = focusable[focusable.length - 1];
      if (e.shiftKey && document.activeElement === first) (e.preventDefault(), last.focus());
      else if (!e.shiftKey && document.activeElement === last) (e.preventDefault(), first.focus());
    };
    document.addEventListener("keydown", onKey);
    return () => {
      document.removeEventListener("keydown", onKey);
      previous?.focus();
    };
  }, [onClose, page]);

  const hasBoth = !!trackComplete && !!levelUp;
  return (
    <div className="fixed inset-0 z-50 flex items-center justify-center bg-bg/80 p-4 backdrop-blur-sm" onMouseDown={(e) => e.target === e.currentTarget && onClose()}>
      <div ref={ref} role="dialog" aria-modal="true" aria-labelledby={titleId} className="celebrate-in panel relative w-full max-w-md overflow-hidden p-7 text-center">
        {confetti && <Confetti />}
        {page === "track" && trackComplete ? (
          <>
            <p className="text-sm font-semibold text-accent">Course complete</p>
            <h2 id={titleId} className="display mt-2 text-3xl font-bold">
              {trackComplete.title}
            </h2>
            <p className="mt-3 text-muted">
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
              <p className="mt-3 text-muted">{levelUp.blurb}</p>
              <p className="mt-2 text-sm text-muted">{levelUp.totalXp} XP in total</p>
              <div className="mt-6 flex flex-col gap-2">
                {trackComplete?.next && page === "level" && hasBoth ? (
                  <Link data-autofocus href={`/learn/${trackComplete.next.slug}`} onClick={onClose} className="inline-flex h-11 items-center justify-center rounded-lg bg-accent px-5 font-medium text-accent-ink hover:brightness-110">
                    Next course: {trackComplete.next.title}
                  </Link>
                ) : (
                  <button type="button" data-autofocus onClick={onClose} className="h-11 rounded-lg bg-accent px-5 font-medium text-accent-ink hover:brightness-110">
                    Keep going
                  </button>
                )}
                {trackComplete?.next && hasBoth && (
                  <button type="button" onClick={onClose} className="h-11 rounded-lg px-5 text-sm text-muted hover:bg-surface-2 hover:text-text">
                    Stay here
                  </button>
                )}
              </div>
            </>
          )
        )}
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
