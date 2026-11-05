"use client";

import { useEffect, useRef, useState } from "react";
import { MiniMarkdown } from "./mini-markdown";

const QUICK_PROMPTS = [
  { label: "Explain this error", text: "Can you explain what the latest error or failing test means, without giving me the answer?" },
  { label: "What is the task asking?", text: "Can you restate what this task is asking me to do, in simpler words?" },
  { label: "Check my approach", text: "Here is my current code. Is my approach on the right track? Don't write the solution for me." },
  { label: "Give me a nudge", text: "I'm stuck. Give me a small nudge about what to look at next." },
] as const;

/**
 * Phone-only help: a floating button that opens a bottom sheet with one-tap tutor prompts and the authored hints, so
 * help is reachable without scrolling to the panels under the editor.
 */
export function HelpSheet({ hints, hintCount, hintBusy, hintError, onOpenHint, onAsk, signedIn, loginHref }: {
  hints: string[];
  hintCount: number;
  hintBusy: boolean;
  hintError: string | null;
  onOpenHint: () => void;
  onAsk: (text: string) => void;
  signedIn: boolean;
  loginHref: string;
}) {
  const [open, setOpen] = useState(false);
  const opener = useRef<HTMLButtonElement>(null);
  const close = useRef<HTMLButtonElement>(null);
  const panel = useRef<HTMLDivElement>(null);

  useEffect(() => {
    if (!open) return;
    close.current?.focus();
    const onKey = (e: KeyboardEvent) => {
      if (e.key === "Escape") setOpen(false);
      if (e.key !== "Tab" || !panel.current) return;
      const items = [...panel.current.querySelectorAll<HTMLElement>("button:not([disabled]), a[href]")];
      if (!items.length) return;
      const first = items[0], last = items[items.length - 1];
      if (e.shiftKey && document.activeElement === first) { e.preventDefault(); last.focus(); }
      else if (!e.shiftKey && document.activeElement === last) { e.preventDefault(); first.focus(); }
    };
    document.addEventListener("keydown", onKey);
    const opening = opener.current;
    return () => { document.removeEventListener("keydown", onKey); opening?.focus({ preventScroll: true }); };
  }, [open]);

  const ask = (text: string) => { setOpen(false); onAsk(text); };
  const btn = "inline-flex min-h-11 items-center rounded-lg border border-border px-4 text-sm hover:bg-surface-2";

  return (
    <div className="lg:hidden">
      <button ref={opener} type="button" aria-haspopup="dialog" onClick={() => setOpen(true)}
        className="fixed bottom-3 right-3 z-30 inline-flex min-h-11 items-center gap-2 rounded-full border border-border bg-surface px-4 text-sm font-medium text-text shadow-lg hover:bg-surface-2">
        <span aria-hidden="true">?</span> Help
      </button>
      {open && (
        <div className="fixed inset-0 z-50 flex items-end bg-bg/70" onClick={() => setOpen(false)}>
          <div ref={panel} role="dialog" aria-modal="true" aria-labelledby="help-sheet-title" onClick={(e) => e.stopPropagation()}
            className="max-h-[80dvh] w-full overflow-y-auto rounded-t-2xl border border-border bg-surface p-4 pb-[max(1rem,env(safe-area-inset-bottom))] shadow-2xl">
            <div className="flex items-center justify-between gap-3">
              <h2 id="help-sheet-title" className="display-sm text-lg font-semibold">Stuck? Start here</h2>
              <button ref={close} type="button" onClick={() => setOpen(false)} className={btn}>Close</button>
            </div>
            <p className="mt-3 text-sm text-muted">Hints are free. The tutor guides you but never hands over the answer.</p>
            <div className="mt-3 flex flex-wrap gap-2">
              {hints.length < hintCount && (
                <button type="button" disabled={hintBusy} onClick={onOpenHint} className={`${btn} border-accent bg-accent-soft text-text disabled:opacity-60`}>
                  {hintBusy ? "Loading hint…" : hints.length === 0 ? "Show a hint" : "Show the next hint"}
                </button>
              )}
            </div>
            {hintError && <p role="alert" className="mt-3 text-sm text-err">{hintError}</p>}
            {hints.length > 0 && (
              <ol className="mt-3 space-y-3">
                {hints.map((h, i) => (
                  <li key={i} className="rounded-lg border border-border bg-bg p-3 text-sm">
                    <p className="mb-1 text-xs font-medium text-muted">Hint {i + 1}</p>
                    <MiniMarkdown text={h} />
                  </li>
                ))}
              </ol>
            )}
            <h3 className="mt-5 text-sm font-semibold">Ask the tutor</h3>
            {signedIn ? (
              <div className="mt-2 flex flex-wrap gap-2">
                {QUICK_PROMPTS.map((p) => <button key={p.label} type="button" onClick={() => ask(p.text)} className={btn}>{p.label}</button>)}
              </div>
            ) : (
              <p className="mt-2 text-sm text-muted"><a className="text-accent underline underline-offset-2" href={loginHref}>Sign in</a> to use the AI tutor.</p>
            )}
          </div>
        </div>
      )}
    </div>
  );
}
