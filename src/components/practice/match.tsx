"use client";

import { useState } from "react";
import type { KindProps } from "./types";

/**
 * Match pairs without dragging: tap a term on the left, then its partner on the right. Tap a matched term to undo it.
 * The submission is the right-hand text chosen for each left term, in order.
 */
export default function MatchPractice({ practice, locked, onAnswer }: KindProps<"match">) {
  const { lefts, rights } = practice;
  const [pairs, setPairs] = useState<(number | null)[]>(() => lefts.map(() => null)); // left index -> right index
  const [active, setActive] = useState<number | null>(null);
  const used = new Set(pairs.filter((v): v is number => v !== null));

  function commit(next: (number | null)[]) {
    setPairs(next);
    onAnswer(next.every((v) => v !== null) ? next.map((v) => rights[v as number]) : null);
  }
  function tapLeft(i: number) {
    if (locked) return;
    if (pairs[i] !== null) { commit(pairs.map((v, j) => (j === i ? null : v))); setActive(i); return; }
    setActive(active === i ? null : i);
  }
  function tapRight(r: number) {
    if (locked || used.has(r)) return;
    const target = active ?? pairs.findIndex((v) => v === null);
    if (target < 0) return;
    commit(pairs.map((v, j) => (j === target ? r : v)));
    setActive(null);
  }
  const slotOf = (r: number) => pairs.findIndex((v) => v === r);

  return (
    <div className="grid gap-4 sm:grid-cols-2">
      <div>
        <p className="text-xs text-muted">Terms</p>
        <ul className="mt-1 grid gap-2">
          {lefts.map((l, i) => (
            <li key={i}>
              <button type="button" aria-pressed={active === i} aria-disabled={locked} onClick={() => tapLeft(i)}
                className={`min-h-11 w-full rounded-lg border px-3 py-2 text-left text-sm ${active === i ? "border-accent bg-accent-soft ring-1 ring-accent" : pairs[i] !== null ? "border-line-strong bg-surface-2" : "border-line-strong bg-surface hover:bg-surface-2"}`}>
                <span className="font-mono">{l}</span>
                {pairs[i] !== null && <span className="mt-1 block text-xs text-text-2">↳ {rights[pairs[i]!]} <span className="text-muted">(tap to undo)</span></span>}
              </button>
            </li>
          ))}
        </ul>
      </div>
      <div>
        <p className="text-xs text-muted">{active !== null ? `Partner for “${lefts[active]}”` : "Partners"}</p>
        <ul className="mt-1 grid gap-2">
          {rights.map((t, r) => (
            <li key={r}>
              <button type="button" disabled={used.has(r)} aria-disabled={locked || used.has(r)} onClick={() => tapRight(r)}
                className="min-h-11 w-full rounded-lg border border-line-strong bg-surface px-3 py-2 text-left text-sm hover:bg-surface-2 disabled:opacity-40">
                {t}{used.has(r) && <span className="sr-only"> (matched with {lefts[slotOf(r)]})</span>}
              </button>
            </li>
          ))}
        </ul>
      </div>
    </div>
  );
}
