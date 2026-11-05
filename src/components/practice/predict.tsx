"use client";

import { useState } from "react";
import { highlight } from "./highlight";
import { TraceVisualiser } from "./trace-visualiser";
import type { KindProps } from "./types";

/** Predict the output (multiple choice, one index) or trace register and variable values (one index per step). */
export default function PredictPractice(props: KindProps<"predict">) {
  const { practice } = props;
  if (practice.trace) return <TraceVisualiser {...props} trace={practice.trace} />;
  return <Choose {...props} />;
}

function Choose({ practice, language, locked, onAnswer }: KindProps<"predict">) {
  const [picked, setPicked] = useState<number | null>(null);
  return (
    <div>
      <p className="text-sm text-text-2">{practice.prompt}</p>
      {practice.code && <pre tabIndex={0} className="mt-3 overflow-x-auto rounded-xl border border-line-strong bg-[var(--code-bg)] p-3 font-mono text-[13px] text-[var(--code-text)]"><code>{highlight(practice.code, language)}</code></pre>}
      <div role="radiogroup" aria-label="Choose the answer" className="mt-3 grid gap-2">
        {(practice.choices ?? []).map((c, i) => {
          const on = picked === i;
          return (
            <button
              key={i}
              type="button"
              role="radio"
              aria-checked={on}
              aria-disabled={locked}
              onClick={() => { if (locked) return; setPicked(i); onAnswer(i); }}
              className={`min-h-11 rounded-lg border px-3 py-2 text-left font-mono text-sm ${on ? "border-accent bg-accent-soft ring-1 ring-accent" : "border-line-strong bg-surface hover:bg-surface-2"}`}
            >
              <span className="whitespace-pre-wrap break-words">{on && <span aria-hidden="true">● </span>}{c}</span>
            </button>
          );
        })}
      </div>
    </div>
  );
}
