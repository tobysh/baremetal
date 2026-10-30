"use client";

import { useCallback, useState } from "react";
import type { PublicExercise } from "@/lib/curriculum/types";
import { MiniMarkdown } from "./mini-markdown";
import type { ChatMessage } from "./tutor-chat";
import { Workspace, type Variant } from "./workspace";

export type ChallengeInfo = { exercise: PublicExercise; code: string | null; passed: boolean; xp: number };
export type ReviewInfo = { due: boolean; dueOn: string };

/**
 * One editor, up to three ways to use it: the lesson, its optional challenge (unlocked by passing the lesson) and
 * a review (re-solving a passed lesson from a fresh starter, which never earns XP).
 */
export function LessonWorkspace(props: {
  lesson: PublicExercise;
  lessonCode: string | null;
  lessonPassed: boolean;
  initialHints: number;
  challenge: ChallengeInfo | null;
  review: ReviewInfo | null;
  startInReview: boolean;
  signedIn: boolean;
  draftOwner?: string;
  tutorHistory: ChatMessage[];
  nextHref: string | null;
  nextLabel?: string;
  loginHref: string;
}) {
  const { lesson, challenge, review, signedIn } = props;
  const [passed, setPassed] = useState(props.lessonPassed);
  const [variant, setVariant] = useState<Variant>(props.startInReview && review && props.lessonPassed ? "review" : "lesson");
  const [refreshed, setRefreshed] = useState(false);
  const onPassed = useCallback(() => setPassed(true), []);
  const showReview = !!review && passed;
  const showChallenge = !!challenge && signedIn;

  const tab = (v: Variant, label: string, enabled: boolean, note?: string) => (
    <button
      key={v}
      type="button"
      aria-pressed={variant === v}
      disabled={!enabled}
      title={enabled ? undefined : "Pass the lesson to unlock this"}
      onClick={() => setVariant(v)}
      className={`min-h-9 rounded-md px-3 text-sm ${variant === v ? "bg-surface-2 font-medium text-text" : "text-muted hover:text-text"} disabled:cursor-not-allowed disabled:opacity-50 disabled:hover:text-muted`}
    >
      {label}
      {note && <span className="ml-1.5 text-xs text-accent">{note}</span>}
    </button>
  );

  return (
    <div className="min-w-0 lg:flex lg:h-full lg:flex-col">
      {(showChallenge || showReview) && (
        <div role="group" aria-label="What to work on" className="mb-2 flex flex-wrap items-center gap-1">
          {tab("lesson", "Lesson", true)}
          {showChallenge && tab("challenge", "Challenge", passed, passed ? `+${challenge.xp} XP` : undefined)}
          {showReview && tab("review", "Review", true, review.due ? "due" : undefined)}
        </div>
      )}
      {variant !== "challenge" && (
        // The task lives in the other view on small screens; keep it one tap away while coding.
        <details className="mb-2 panel text-sm lg:hidden">
          <summary className="min-h-11 cursor-pointer px-4 py-3 font-medium">Task</summary>
          <div className="px-4 pb-4"><MiniMarkdown text={lesson.task} /></div>
        </details>
      )}
      {variant === "challenge" && challenge && (
        <section aria-label="Challenge" className="mb-2 panel p-4 text-sm">
          <p className="text-xs font-semibold text-accent">Challenge · optional · {challenge.passed ? "passed" : `worth ${challenge.xp} XP`}</p>
          <div className="mt-2">
            <MiniMarkdown text={challenge.exercise.task} />
          </div>
        </section>
      )}
      {variant === "review" && review && (
        <section aria-label="Review" className="mb-2 panel p-4 text-sm">
          <p className="text-xs font-semibold text-accent">Review · no XP</p>
          <p className="mt-1 text-muted">
            Solve this again from a fresh start; your saved solution is untouched. {review.due ? "It's due now." : `Next review is due ${review.dueOn}; passing early is just practice.`}
          </p>
          <button
            type="button"
            disabled={refreshed}
            className="mt-2 min-h-9 rounded-md border border-border px-3 text-xs hover:bg-surface-2 disabled:opacity-60"
            onClick={async () => {
              const res = await fetch("/api/progress", { method: "POST", headers: { "Content-Type": "application/json" }, body: JSON.stringify({ exerciseId: lesson.id, event: "review-reset" }) }).catch(() => null);
              if (res?.ok) setRefreshed(true);
            }}
          >
            {refreshed ? "Okay, we'll ask again in a few days" : "I need a refresher"}
          </button>
        </section>
      )}
      <Workspace
        key={`${lesson.id}:${variant}`}
        exercise={variant === "challenge" && challenge ? challenge.exercise : lesson}
        initialCode={variant === "challenge" ? (challenge?.code ?? null) : variant === "review" ? null : props.lessonCode}
        signedIn={signedIn}
        draftOwner={props.draftOwner}
        alreadyPassed={variant === "challenge" ? !!challenge?.passed : variant === "review" ? false : passed}
        tutorHistory={variant === "challenge" ? [] : props.tutorHistory}
        nextHref={props.nextHref}
        nextLabel={props.nextLabel}
        loginHref={props.loginHref}
        variant={variant}
        initialHints={props.initialHints}
        onPassed={onPassed}
      />
    </div>
  );
}
