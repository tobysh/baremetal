"use client";

import type { ReactNode } from "react";

export type LessonTarget = "lesson" | "code";
export const jumpEvent = "bm:lesson-jump";

/** Asks LessonLayout to show a pane and move focus into it. */
export function jumpTo(target: LessonTarget) {
  window.dispatchEvent(new CustomEvent<LessonTarget>(jumpEvent, { detail: target }));
}

export function LessonJump({ target, className, children }: { target: LessonTarget; className?: string; children: ReactNode }) {
  return (
    <button type="button" onClick={() => jumpTo(target)} className={className}>
      {children}
    </button>
  );
}
