"use client";

import { useState } from "react";
import { highlight } from "./highlight";
import type { KindProps } from "./types";

/** Concept question. One answer is a radio group; "select all that apply" turns into checkboxes. The submission is the chosen indices. */
export default function ChoicePractice({ practice, language, locked, onAnswer }: KindProps<"choice">) {
  const [picked, setPicked] = useState<number[]>([]);
  const multi = practice.multi;

  function toggle(i: number) {
    if (locked) return;
    const next = multi ? (picked.includes(i) ? picked.filter((x) => x !== i) : [...picked, i]) : [i];
    setPicked(next);
    onAnswer(next.length ? next : null);
  }

  return (
    <div>
      {practice.code && <pre tabIndex={0} className="overflow-x-auto rounded-xl border border-line-strong bg-[var(--code-bg)] p-3 font-mono text-[13px] text-[var(--code-text)]"><code>{highlight(practice.code, language)}</code></pre>}
      <div role={multi ? "group" : "radiogroup"} aria-label={multi ? "Choose every answer that applies" : "Choose the answer"} className="mt-3 grid gap-2">
        {multi && <p className="text-xs text-muted">Select all that apply.</p>}
        {practice.choices.map((c, i) => {
          const on = picked.includes(i);
          return (
            <button
              key={i}
              type="button"
              role={multi ? "checkbox" : "radio"}
              aria-checked={on}
              aria-disabled={locked}
              onClick={() => toggle(i)}
              className={`flex min-h-11 items-start gap-3 rounded-lg border px-3 py-2 text-left text-sm ${on ? "border-accent bg-accent-soft ring-1 ring-accent" : "border-line-strong bg-surface hover:bg-surface-2"}`}
            >
              <span aria-hidden="true" className={`mt-0.5 grid size-4 shrink-0 place-items-center border border-line-strong text-[10px] leading-none ${multi ? "rounded" : "rounded-full"} ${on ? "border-accent bg-accent text-accent-ink" : ""}`}>{on ? (multi ? "✓" : "●") : ""}</span>
              <span className="whitespace-pre-wrap break-words">{c}</span>
            </button>
          );
        })}
      </div>
    </div>
  );
}
