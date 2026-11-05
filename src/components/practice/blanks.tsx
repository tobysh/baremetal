"use client";

import { useEffect, useMemo, useRef, useState, type PointerEvent as ReactPointerEvent } from "react";
import { highlight } from "./highlight";
import type { KindProps } from "./types";

const SLOT = /\{\{(\d+)\}\}/g;
const SLOP_PX = 8;

type Seg = { text: string } | { slot: number };

/**
 * Fill the blanks in a snippet. Tap a blank, then tap a piece from the tray (or just tap pieces and they fill the
 * next empty blank); tap a filled blank to put its piece back; or drag a piece onto a blank. Submits one piece
 * per blank, in blank order.
 */
export default function Blanks({ practice, language, locked, onAnswer }: KindProps<"blanks">) {
  const segs = useMemo<Seg[]>(() => {
    const out: Seg[] = [];
    let last = 0;
    for (const m of practice.template.matchAll(SLOT)) {
      if (m.index > last) out.push({ text: practice.template.slice(last, m.index) });
      out.push({ slot: Number(m[1]) });
      last = m.index + m[0].length;
    }
    if (last < practice.template.length) out.push({ text: practice.template.slice(last) });
    return out;
  }, [practice.template]);

  // filled[slot] = index into practice.tokens, or null.
  const [filled, setFilled] = useState<(number | null)[]>(() => Array.from({ length: practice.slots }, () => null));
  const [selected, setSelected] = useState<number | null>(0);
  const [announce, setAnnounce] = useState("");
  const [ghost, setGhost] = useState<{ x: number; y: number; token: number } | null>(null);
  const filledRef = useRef(filled);
  const suppressClick = useRef(false);
  const dragRef = useRef<{ token: number; pointerId: number; x: number; y: number; moved: boolean } | null>(null);

  const publish = (next: (number | null)[]) => {
    filledRef.current = next;
    setFilled(next);
    onAnswer(next.every((v) => v !== null) ? next.map((v) => practice.tokens[v!]) : null);
  };

  function place(token: number, slot: number | null) {
    if (locked) return;
    const cur = filledRef.current;
    const target = slot ?? selected ?? cur.indexOf(null);
    if (target < 0 || cur[target] === undefined || cur.includes(token)) return;
    const next = cur.slice();
    next[target] = token;
    publish(next);
    const nextEmpty = next.findIndex((v, i) => v === null && i !== target);
    setSelected(nextEmpty >= 0 ? nextEmpty : null);
    setAnnounce(`Blank ${target + 1} filled with ${practice.tokens[token]}.${nextEmpty >= 0 ? ` Blank ${nextEmpty + 1} is next.` : " All blanks are filled."}`);
  }
  function tapSlot(slot: number) {
    if (locked) return;
    const cur = filledRef.current;
    if (cur[slot] !== null) {
      const next = cur.slice();
      next[slot] = null;
      publish(next);
      setAnnounce(`Blank ${slot + 1} cleared.`);
    }
    setSelected(slot);
  }
  const clearAll = () => { publish(Array.from({ length: practice.slots }, () => null)); setSelected(0); setAnnounce("All blanks cleared."); };

  // ----- Dragging a token onto a blank -----
  function tokenDown(e: ReactPointerEvent<HTMLButtonElement>, token: number) {
    if (locked || filled.includes(token) || (e.pointerType === "mouse" && e.button !== 0)) return;
    dragRef.current = { token, pointerId: e.pointerId, x: e.clientX, y: e.clientY, moved: false };
  }
  useEffect(() => {
    const move = (e: PointerEvent) => {
      const d = dragRef.current;
      if (!d || e.pointerId !== d.pointerId) return;
      if (!d.moved && Math.hypot(e.clientX - d.x, e.clientY - d.y) > SLOP_PX) d.moved = true;
      if (d.moved) setGhost({ x: e.clientX, y: e.clientY, token: d.token });
    };
    const end = (e: PointerEvent, drop: boolean) => {
      const d = dragRef.current;
      if (!d || e.pointerId !== d.pointerId) return;
      dragRef.current = null;
      setGhost(null);
      if (!d.moved) return;
      suppressClick.current = true;
      setTimeout(() => { suppressClick.current = false; }, 0);
      if (!drop) return;
      const slot = document.elementFromPoint(e.clientX, e.clientY)?.closest<HTMLElement>("[data-slot]");
      if (slot) place(d.token, Number(slot.dataset.slot));
    };
    const up = (e: PointerEvent) => end(e, true);
    const cancel = (e: PointerEvent) => end(e, false);
    window.addEventListener("pointermove", move);
    window.addEventListener("pointerup", up);
    window.addEventListener("pointercancel", cancel);
    return () => {
      window.removeEventListener("pointermove", move);
      window.removeEventListener("pointerup", up);
      window.removeEventListener("pointercancel", cancel);
    };
  });

  const slotBtn = (slot: number) => {
    const t = filled[slot];
    const isSel = selected === slot;
    return (
      <button
        key={`s${slot}`}
        type="button"
        data-slot={slot}
        disabled={locked}
        onClick={() => tapSlot(slot)}
        aria-label={t === null ? `Blank ${slot + 1}, empty${isSel ? ", selected" : ""}` : `Blank ${slot + 1}: ${practice.tokens[t]}. Press to clear.`}
        aria-pressed={isSel}
        className={`mx-0.5 inline-flex min-h-11 min-w-11 items-center justify-center rounded-md border px-2 align-middle font-mono text-[13px] ${t === null ? "border-dashed" : "border-solid"} ${isSel ? "border-accent bg-accent-soft" : "border-border bg-surface-2"} disabled:opacity-60`}
      >
        {t === null ? <span aria-hidden="true" className="text-muted">{slot + 1}</span> : <span style={{ color: "var(--code-text)" }}>{practice.tokens[t]}</span>}
      </button>
    );
  };

  return (
    <div className="space-y-4">
      <div className="overflow-x-auto rounded-lg border border-border p-3" style={{ background: "var(--code-bg)", color: "var(--code-text)" }}>
        <pre className="font-mono text-[13px] leading-6 whitespace-pre">
          {segs.map((s, i) => ("slot" in s ? slotBtn(s.slot) : <span key={`t${i}`}>{highlight(s.text, language, `${i}-`)}</span>))}
        </pre>
      </div>

      <div>
        <div className="flex items-center justify-between gap-2">
          <h3 className="text-sm font-semibold">Pieces <span className="font-normal text-muted">(tap one to fill {selected === null ? "the next blank" : `blank ${selected + 1}`})</span></h3>
          <button type="button" onClick={clearAll} disabled={locked || filled.every((v) => v === null)} className="inline-flex min-h-11 items-center rounded-lg border border-border bg-surface px-3 text-sm font-medium hover:bg-surface-2 disabled:opacity-50">Clear all</button>
        </div>
        <ul className="mt-2 flex flex-wrap gap-2">
          {practice.tokens.map((tok, i) => {
            const used = filled.includes(i);
            return (
              <li key={i}>
                <button
                  type="button"
                  disabled={locked || used}
                  onPointerDown={(e) => tokenDown(e, i)}
                  onClick={() => { if (!suppressClick.current) place(i, null); }}
                  aria-label={used ? `${tok} (placed)` : `Piece ${tok}`}
                  className="min-h-11 min-w-11 touch-none select-none rounded-lg border border-border bg-surface-2 px-3 font-mono text-[13px] hover:border-accent disabled:opacity-40"
                >
                  {tok}
                </button>
              </li>
            );
          })}
        </ul>
      </div>
      {ghost && (
        <div aria-hidden="true" className="pointer-events-none fixed z-50 -translate-x-1/2 -translate-y-full rounded-lg border border-accent bg-surface-2 px-3 py-2 font-mono text-[13px] shadow-lg" style={{ left: ghost.x, top: ghost.y - 8 }}>
          {practice.tokens[ghost.token]}
        </div>
      )}
      <p role="status" aria-live="polite" className="sr-only">{announce}</p>
    </div>
  );
}
