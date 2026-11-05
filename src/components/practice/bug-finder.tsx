"use client";

import { useState } from "react";
import { highlight } from "./highlight";
import type { KindProps } from "./types";

/** Find the bug: tap the faulty line of a read-only snippet. The shell sends the 1-based line number; the answer never comes down. */
export default function BugFinder({ practice, language, locked, onAnswer }: KindProps<"bug">) {
  const lines = practice.code.replace(/\n$/, "").split("\n");
  const [picked, setPicked] = useState<number | null>(null);

  function pick(n: number) {
    if (locked) return;
    setPicked(n);
    onAnswer(n);
  }

  return (
    <div>
      <p className="text-sm text-text-2">{practice.prompt}</p>
      <ol aria-label="Code, one button per line" className="mt-3 overflow-hidden rounded-xl border border-line-strong bg-[var(--code-bg)] font-mono text-[13px] text-[var(--code-text)]">
        {lines.map((text, i) => {
          const n = i + 1;
          const on = picked === n;
          return (
            <li key={n}>
              <button
                type="button"
                aria-pressed={on}
                aria-disabled={locked}
                onClick={() => pick(n)}
                className={`flex min-h-8 w-full items-start gap-3 px-3 py-1 text-left pointer-coarse:min-h-11 pointer-coarse:items-center ${on ? "bg-accent-soft ring-2 ring-inset ring-accent" : "hover:bg-[var(--code-active)]"}`}
              >
                <span aria-hidden="true" className="w-6 shrink-0 select-none text-right text-[var(--code-muted)]">{n}</span>
                <span className="sr-only">Line {n}{on ? ", selected" : ""}: </span>
                <code className="min-w-0 flex-1 whitespace-pre-wrap break-words">{text ? highlight(text, language, `${n}-`) : " "}</code>
              </button>
            </li>
          );
        })}
      </ol>
      <p className="mt-2 text-xs text-muted">{picked == null ? "Tap the line you think is wrong." : `Line ${picked} selected. Tap another line to change it.`}</p>
    </div>
  );
}
