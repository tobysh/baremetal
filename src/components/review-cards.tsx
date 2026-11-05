"use client";

import Link from "next/link";
import { Suspense, useState } from "react";
import type { Language } from "@/lib/curriculum/types";
import type { PracticeSubmission, PublicPractice } from "@/lib/curriculum/practice";
import { MiniMarkdown } from "./workspace/mini-markdown";
import { practiceKinds } from "./practice/registry";
import type { KindProps } from "./practice/types";
import { Skeleton } from "./ui";

export type ReviewCard = {
  exerciseId: string;
  title: string;
  trackTitle: string;
  summary: string;
  /** Opens the lesson's coding review. */
  href: string;
  language: Language;
  /** The question to answer, or null for a lesson that has none (it gets a "write it again" card). */
  question: { q: number; practice: PublicPractice } | null;
};

type Result = { correct: boolean; feedback: string | null; explain: string | null; reveal: string | null };

/**
 * Review cards ask a real question (any practice kind: multiple choice, type the answer, match, predict, fill, arrange,
 * find the bug) and grade it on the server, so remembering is tested instead of self-reported. The first answer settles
 * the schedule. Lessons with no question get a card that sends the learner to write the solution again.
 */
export function ReviewCards({ cards }: { cards: ReviewCard[] }) {
  const [index, setIndex] = useState(0);
  const [tally, setTally] = useState({ right: 0, wrong: 0, coding: 0 });
  const card = cards[index];

  if (!card) {
    return (
      <div className="panel mt-6 p-6" role="status">
        <p className="display-sm text-lg font-bold">All caught up</p>
        <p className="mt-1 text-sm text-muted">{tally.right} right, {tally.wrong} to see again soon{tally.coding ? `, ${tally.coding} left for you to write out` : ""}.</p>
        <Link href="/dashboard" className="mt-4 inline-flex min-h-11 items-center rounded-lg border border-line-strong px-4 text-sm font-medium hover:bg-surface-2">Back to your dashboard</Link>
      </div>
    );
  }
  return (
    <div className="mt-6">
      <p className="text-sm text-muted" aria-live="polite">Question {index + 1} of {cards.length}</p>
      <article className="panel mt-2 p-5" aria-label={card.title}>
        <p className="text-xs text-muted">{card.trackTitle}</p>
        <h2 className="display-sm mt-1 text-xl font-bold">{card.title}</h2>
        {card.question ? (
          <Question key={card.exerciseId} card={card} q={card.question} onDone={(correct) => { setTally((t) => ({ ...t, [correct ? "right" : "wrong"]: t[correct ? "right" : "wrong"] + 1 })); setIndex((i) => i + 1); }} />
        ) : (
          <div key={card.exerciseId}>
            <p className="mt-2 text-sm text-text-2">{card.summary}</p>
            <p className="mt-3 text-sm">This lesson has no quick question yet, so the test is writing the solution again from memory. Passing it there moves it along; leaving it keeps it due.</p>
            <div className="mt-4 flex flex-wrap gap-3">
              <Link href={card.href} className="inline-flex min-h-11 items-center rounded-lg bg-accent px-4 text-sm font-semibold text-accent-ink">Write it again →</Link>
              <button type="button" onClick={() => { setTally((t) => ({ ...t, coding: t.coding + 1 })); setIndex((i) => i + 1); }} className="min-h-11 rounded-lg border border-line-strong px-4 text-sm font-medium hover:bg-surface-2">Skip for now</button>
            </div>
          </div>
        )}
      </article>
    </div>
  );
}

function Question({ card, q, onDone }: { card: ReviewCard; q: NonNullable<ReviewCard["question"]>; onDone: (correct: boolean) => void }) {
  const [submission, setSubmission] = useState<PracticeSubmission | null>(null);
  const [busy, setBusy] = useState(false);
  const [result, setResult] = useState<Result | null>(null);
  const [error, setError] = useState<string | null>(null);
  const Kind = practiceKinds[q.practice.kind] as React.ComponentType<KindProps>;

  async function check() {
    if (!submission || busy || result) return;
    setBusy(true);
    setError(null);
    try {
      const res = await fetch("/api/review", { method: "POST", headers: { "Content-Type": "application/json" }, body: JSON.stringify({ exerciseId: card.exerciseId, q: q.q, submission }) });
      const body = await res.json().catch(() => ({}));
      if (!res.ok) {
        setError(res.status === 409 ? "That review was already answered. Move on to the next one." : (body.error as string | undefined) ?? "Could not save that answer. Try again.");
        return;
      }
      setResult(body as Result);
    } catch {
      setError("Could not reach the server. Check your connection and try again.");
    } finally {
      setBusy(false);
    }
  }

  return (
    <div>
      {/* predict and bug draw their own prompt */}
      {q.practice.kind !== "predict" && q.practice.kind !== "bug" && <div className="mt-3 text-sm"><MiniMarkdown text={q.practice.prompt} /></div>}
      <div className="mt-4">
        <Suspense fallback={<div aria-busy="true" className="space-y-2"><Skeleton className="h-11 w-full" /><Skeleton className="h-11 w-2/3" /></div>}>
          <Kind practice={q.practice as KindProps["practice"]} language={card.language} locked={busy || !!result} attempt={0} onAnswer={(s) => { if (!result) setSubmission(s); }} />
        </Suspense>
      </div>
      {!result ? (
        <button type="button" onClick={check} disabled={!submission || busy} className="mt-5 inline-flex min-h-11 items-center rounded-lg bg-accent px-5 text-sm font-semibold text-accent-ink disabled:cursor-not-allowed disabled:opacity-50">
          {busy ? "Checking…" : "Check answer"}
        </button>
      ) : (
        <div role="status" aria-live="polite" className={`mt-5 rounded-lg border border-border p-3 text-sm ${result.correct ? "bg-ok-soft" : "bg-err-soft"}`}>
          <p className={`font-medium ${result.correct ? "text-ok" : "text-err"}`}>{result.correct ? "Right. This one comes back later." : "Not this time. It comes back in a few days."}</p>
          {!result.correct && result.reveal && (
            <div className="mt-2"><p className="text-xs text-muted">The answer</p><pre className="mt-1 overflow-x-auto whitespace-pre-wrap rounded-md bg-surface-2 p-2 font-mono text-[13px]">{result.reveal}</pre></div>
          )}
          {result.explain && <div className="mt-2"><MiniMarkdown text={result.explain} /></div>}
          <div className="mt-3 flex flex-wrap gap-3">
            <button type="button" autoFocus onClick={() => onDone(result.correct)} className="min-h-11 rounded-lg bg-accent px-4 text-sm font-semibold text-accent-ink">Next →</button>
            {!result.correct && <Link href={card.href} className="inline-flex min-h-11 items-center text-sm text-accent underline underline-offset-2">Open the lesson to relearn it</Link>}
          </div>
        </div>
      )}
      {error && <p role="alert" className="mt-3 rounded-lg border border-err bg-err-soft p-3 text-sm text-text">{error}</p>}
    </div>
  );
}
