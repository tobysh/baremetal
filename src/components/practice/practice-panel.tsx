"use client";

import Link from "next/link";
import { useRouter } from "next/navigation";
import { Suspense, useRef, useState } from "react";
import type { Language } from "@/lib/curriculum/types";
import type { PracticeSubmission, PublicPractice } from "@/lib/curriculum/practice";
import { Celebration, type Reward } from "../celebrations";
import { MiniMarkdown } from "../workspace/mini-markdown";
import { Skeleton } from "../ui";
import { practiceKinds } from "./registry";
import type { KindProps } from "./types";

type Outcome = { saved: boolean; firstPass: boolean; xp: number; xpWorth: number; paced: boolean; reward: Reward | null };
type Status =
  | { s: "idle" }
  | { s: "checking" }
  | { s: "wrong"; feedback: string | null }
  | { s: "error"; message: string }
  | { s: "correct"; explain: string | null; outcome: Outcome | null };

const KIND_LABEL: Record<PublicPractice["kind"], string> = { parsons: "Arrange the lines", blanks: "Fill the blanks", predict: "Predict the output", bug: "Find the bug", choice: "Multiple choice", typed: "Type the answer", match: "Match the pairs" };

/**
 * Shell for every practice kind: the prompt, the XP note, the Check button, feedback and the reward. The
 * interaction itself comes from `practiceKinds` (see ./registry.ts). Answers are checked on the server; this
 * component only ever holds what the learner chose.
 */
export function PracticePanel({ practice, language, passed, signedIn, loginHref, onPassed }: {
  practice: PublicPractice;
  language: Language;
  passed: boolean;
  signedIn: boolean;
  loginHref: string;
  onPassed?: () => void;
}) {
  const router = useRouter();
  const [round, setRound] = useState(0); // bumped by "Practice again" to remount the kind with a fresh board
  const [submission, setSubmission] = useState<PracticeSubmission | null>(null);
  const [status, setStatus] = useState<Status>({ s: "idle" });
  const [attempt, setAttempt] = useState(0);
  const [done, setDone] = useState(passed);
  const [levelUp, setLevelUp] = useState<Reward | null>(null);
  const checkRef = useRef<HTMLButtonElement>(null);
  const Kind = practiceKinds[practice.kind] as React.ComponentType<KindProps>;

  const correct = status.s === "correct";
  const locked = status.s === "checking" || correct;

  async function check() {
    if (!submission || locked) return;
    setStatus({ s: "checking" });
    try {
      const res = await fetch("/api/practice", { method: "POST", headers: { "Content-Type": "application/json" }, body: JSON.stringify({ exerciseId: practice.id, submission }) });
      const body = await res.json().catch(() => ({}));
      if (!res.ok) {
        setStatus({ s: "error", message: res.status === 401 ? "Sign in to check your answer and save your progress." : res.status === 503 ? "The checker is busy. Try again in a moment." : res.status === 429 ? "You're going a bit fast. Wait a few seconds and try again." : (body.error as string | undefined) ?? "Something went wrong checking your answer." });
        return;
      }
      if (body.ok) {
        const outcome = (body.outcome as Outcome | undefined) ?? null;
        setStatus({ s: "correct", explain: (body.explain as string | null | undefined) ?? null, outcome });
        setDone(true);
        onPassed?.();
        if (outcome?.reward) router.refresh(); // header XP chip and dashboard read the user at request time
        if (outcome?.reward?.levelUp) setLevelUp(outcome.reward);
      } else {
        setAttempt((n) => n + 1);
        setStatus({ s: "wrong", feedback: (body.feedback as string | undefined) ?? null });
      }
    } catch {
      setStatus({ s: "error", message: "Could not reach the server. Check your connection and try again." });
    }
  }

  const props: KindProps = {
    practice: practice as KindProps["practice"],
    language,
    locked,
    attempt,
    onAnswer: (s) => {
      setSubmission(s);
      setStatus((cur) => (cur.s === "checking" || cur.s === "correct" ? cur : { s: "idle" }));
    },
  };

  return (
    <section aria-label="Practice" className="panel p-4 text-sm">
      <p className="text-xs font-semibold text-accent">Practice · {KIND_LABEL[practice.kind]}{done && " · completed"}</p>
      <p className="mt-1 text-muted">
        Worth {practice.xp} XP, against {practice.fullXp} XP for the full coding task. Practice never replaces the task, and passing the task later still earns its full XP.
      </p>
      <div className="mt-3"><MiniMarkdown text={practice.prompt} /></div>

      <div className="mt-4">
        <Suspense fallback={<div aria-busy="true" aria-label="Loading the exercise" className="space-y-2"><Skeleton className="h-11 w-full" /><Skeleton className="h-11 w-full" /><Skeleton className="h-11 w-2/3" /></div>}>
          <Kind key={round} {...props} />
        </Suspense>
      </div>

      <div className="mt-5 flex flex-wrap items-center gap-3">
        {!correct ? (
          signedIn ? (
            <button ref={checkRef} type="button" onClick={check} disabled={!submission || locked} className="inline-flex min-h-11 items-center rounded-lg bg-accent px-5 font-semibold text-accent-ink hover:brightness-110 disabled:cursor-not-allowed disabled:opacity-50">
              {status.s === "checking" ? "Checking…" : "Check answer"}
            </button>
          ) : (
            <p>
              <Link href={loginHref} className="underline underline-offset-2 text-accent">Sign in</Link> to check your answer and earn practice XP.
            </p>
          )
        ) : (
          <button type="button" onClick={() => { setRound((n) => n + 1); setSubmission(null); setStatus({ s: "idle" }); setAttempt(0); }} className="inline-flex min-h-11 items-center rounded-lg border border-border px-4 font-medium hover:bg-surface-2">
            Practice again
          </button>
        )}
      </div>

      <div role="status" aria-live="polite" className="mt-3 empty:hidden">
        {status.s === "wrong" && (
          <div className="rounded-lg border border-border bg-err-soft p-3">
            <p className="font-medium text-err">Not quite.</p>
            <p className="mt-1">{status.feedback ?? "Have another look and try again."}</p>
          </div>
        )}
        {status.s === "error" && (
          <div className="rounded-lg border border-border bg-err-soft p-3">
            <p className="text-err">{status.message}</p>
            {!signedIn && <Link href={loginHref} className="mt-1 inline-block underline underline-offset-2">Sign in</Link>}
          </div>
        )}
        {status.s === "correct" && (
          <div className="rounded-lg border border-border bg-ok-soft p-3">
            <p className="font-medium text-ok">
              Correct!
              {status.outcome && status.outcome.xp > 0 && <span className="ml-2 rounded-full bg-accent px-2 py-0.5 text-xs font-semibold text-accent-ink">+{status.outcome.xp} XP</span>}
            </p>
            {status.outcome && !status.outcome.firstPass && <p className="mt-1">You&apos;d already finished this practice, so no more XP this time.</p>}
            {status.outcome?.paced && <p className="mt-1">No XP was added because you answered unusually fast or reached the daily XP limit. XP resumes automatically.</p>}
            {status.outcome?.reward?.levelUp && <p className="mt-1 font-medium">Level up! You&apos;re now level {status.outcome.reward.level}: {status.outcome.reward.title}.</p>}
            {status.explain && <div className="mt-2"><MiniMarkdown text={status.explain} /></div>}
            <p className="mt-2 text-muted">The full coding task is still waiting, and it pays {practice.fullXp} XP.</p>
          </div>
        )}
      </div>
      {levelUp && <Celebration levelUp={levelUp} trackComplete={null} returnFocus={() => checkRef.current} onClose={() => setLevelUp(null)} />}
    </section>
  );
}
