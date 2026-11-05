"use client";

import { useCallback, useEffect, useLayoutEffect, useMemo, useRef, useState, type KeyboardEvent, type PointerEvent as ReactPointerEvent } from "react";
import { highlight } from "./highlight";
import type { KindProps } from "./types";

type Line = { id: number; text: string };

const LONG_PRESS_MS = 320;
const SLOP_PX = 8;

type Drag = { id: number; pointerId: number; grabDy: number; startX: number; startY: number; y: number; tf: number; lifted: boolean; timer: ReturnType<typeof setTimeout> | null };

/**
 * Arrange lines into a program. Tap a line to add it, drag the grip (or press and hold anywhere on a line) to
 * reorder, or use the Move up / Move down / Remove buttons that appear under the selected line. Alt+Arrow
 * also moves the focused line. Submits the chosen lines' text in order.
 */
export default function Parsons({ practice, language, locked, onAnswer }: KindProps<"parsons">) {
  const lines = useMemo<Line[]>(() => practice.lines.map((text, id) => ({ id, text })), [practice.lines]);
  const byId = useMemo(() => new Map(lines.map((l) => [l.id, l])), [lines]);
  const [placed, setPlaced] = useState<number[]>([]);
  const [active, setActive] = useState<number | null>(null);
  const [lifted, setLifted] = useState<number | null>(null);
  const [announce, setAnnounce] = useState("");
  const rows = useRef(new Map<number, HTMLLIElement>());
  const drag = useRef<Drag | null>(null);
  const placedRef = useRef(placed);
  const lockedRef = useRef(locked);
  useEffect(() => { placedRef.current = placed; lockedRef.current = locked; });

  const commit = useCallback((next: number[]) => {
    placedRef.current = next;
    setPlaced(next);
    // Without distractors every line is needed; with them, any non-empty choice can be checked.
    const complete = practice.hasDistractors ? next.length > 0 : next.length === lines.length;
    onAnswer(complete ? next.map((id) => byId.get(id)!.text) : null);
  }, [practice.hasDistractors, lines.length, byId, onAnswer]);

  const pool = lines.filter((l) => !placed.includes(l.id));

  const add = (id: number) => {
    if (locked) return;
    commit([...placed, id]);
    setAnnounce(`Added to position ${placed.length + 1}.`);
  };
  const remove = (id: number) => {
    if (locked) return;
    const i = placed.indexOf(id);
    commit(placed.filter((x) => x !== id));
    setActive(null);
    setAnnounce("Line returned to the list.");
    // Keep keyboard focus in the list instead of dropping it with the removed row.
    requestAnimationFrame(() => {
      const next = placedRef.current[Math.min(i, placedRef.current.length - 1)];
      (next !== undefined ? rows.current.get(next)?.querySelector<HTMLElement>("[data-line]") : document.getElementById(`${practice.id}-pool`)?.querySelector<HTMLElement>("button"))?.focus();
    });
  };
  const moveTo = useCallback((id: number, to: number, say = true) => {
    const cur = placedRef.current;
    const from = cur.indexOf(id);
    const t = Math.max(0, Math.min(cur.length - 1, to));
    if (from < 0 || from === t) return;
    const next = cur.slice();
    next.splice(from, 1);
    next.splice(t, 0, id);
    commit(next);
    if (say) setAnnounce(`Moved to position ${t + 1} of ${next.length}.`);
  }, [commit]);

  // ----- Pointer dragging -----
  const [preventScroll] = useState(() => (e: TouchEvent) => { if (e.cancelable) e.preventDefault(); });
  // Stable wrappers so add/removeEventListener always see the same function; the effect below points them at the latest handlers.
  const latest = useRef<{ move?: (e: PointerEvent) => void; up?: (e: PointerEvent) => void; cancel?: (e: PointerEvent) => void }>({});
  const [handlers] = useState(() => ({
    move: (e: PointerEvent) => latest.current.move?.(e),
    up: (e: PointerEvent) => latest.current.up?.(e),
    cancel: (e: PointerEvent) => latest.current.cancel?.(e),
  }));

  const reposition = useCallback(() => {
    const d = drag.current;
    if (!d?.lifted) return;
    const el = rows.current.get(d.id);
    if (!el) return;
    const rect = el.getBoundingClientRect();
    const natural = rect.top - d.tf;
    const desiredTop = d.y - d.grabDy;
    d.tf = desiredTop - natural;
    el.style.transform = `translateY(${d.tf}px)`;
    const centre = desiredTop + rect.height / 2;
    const others = placedRef.current.filter((x) => x !== d.id);
    let target = 0;
    for (const o of others) {
      const r = rows.current.get(o)?.getBoundingClientRect();
      if (r && r.top + r.height / 2 < centre) target++;
    }
    if (target !== placedRef.current.indexOf(d.id)) moveTo(d.id, target, false);
  }, [moveTo]);

  // After the list reorders, the lifted row's natural position changed: re-anchor it under the finger.
  useLayoutEffect(() => { reposition(); }, [placed, reposition]);

  const endDrag = useCallback((commitMove: boolean) => {
    const d = drag.current;
    if (!d) return;
    if (d.timer) clearTimeout(d.timer);
    window.removeEventListener("pointermove", handlers.move);
    window.removeEventListener("pointerup", handlers.up);
    window.removeEventListener("pointercancel", handlers.cancel);
    window.removeEventListener("touchmove", preventScroll);
    const el = rows.current.get(d.id);
    if (el) el.style.transform = "";
    drag.current = null;
    setLifted(null);
    if (commitMove && d.lifted) {
      const pos = placedRef.current.indexOf(d.id);
      setAnnounce(`Dropped at position ${pos + 1} of ${placedRef.current.length}.`);
    }
  }, [preventScroll, handlers]);

  const lift = useCallback((d: Drag) => {
    if (d.timer) { clearTimeout(d.timer); d.timer = null; }
    d.lifted = true;
    const el = rows.current.get(d.id);
    try { el?.releasePointerCapture(d.pointerId); } catch { /* No implicit capture on mouse. */ }
    window.addEventListener("touchmove", preventScroll, { passive: false });
    try { navigator.vibrate?.(8); } catch { /* Haptics are optional. */ }
    setLifted(d.id);
    setActive(d.id);
    reposition();
  }, [preventScroll, reposition]);

  const onMove = useCallback((e: PointerEvent) => {
    const d = drag.current;
    if (!d || e.pointerId !== d.pointerId) return;
    d.y = e.clientY;
    if (!d.lifted) {
      if (Math.hypot(e.clientX - d.startX, e.clientY - d.startY) > SLOP_PX) {
        // A finger that moves before the press is long enough means "scroll", not "drag".
        if (e.pointerType === "mouse") lift(d); else endDrag(false);
      }
      return;
    }
    reposition();
  }, [lift, endDrag, reposition]);
  const onUp = useCallback((e: PointerEvent) => { if (drag.current && e.pointerId === drag.current.pointerId) endDrag(true); }, [endDrag]);
  const onCancel = useCallback((e: PointerEvent) => { if (drag.current && e.pointerId === drag.current.pointerId) endDrag(false); }, [endDrag]);
  useEffect(() => { latest.current = { move: onMove, up: onUp, cancel: onCancel }; });
  useEffect(() => () => {
    window.removeEventListener("pointermove", handlers.move);
    window.removeEventListener("pointerup", handlers.up);
    window.removeEventListener("pointercancel", handlers.cancel);
    window.removeEventListener("touchmove", preventScroll);
  }, [handlers, preventScroll]);

  function down(e: ReactPointerEvent<HTMLLIElement>, id: number) {
    if (lockedRef.current || drag.current || (e.pointerType === "mouse" && e.button !== 0)) return;
    if ((e.target as HTMLElement).closest("[data-control]")) return;
    const el = rows.current.get(id);
    if (!el) return;
    const d: Drag = { id, pointerId: e.pointerId, grabDy: e.clientY - el.getBoundingClientRect().top, startX: e.clientX, startY: e.clientY, y: e.clientY, tf: 0, lifted: false, timer: null };
    drag.current = d;
    window.addEventListener("pointermove", handlers.move);
    window.addEventListener("pointerup", handlers.up);
    window.addEventListener("pointercancel", handlers.cancel);
    if ((e.target as HTMLElement).closest("[data-grip]")) lift(d);
    else if (e.pointerType !== "mouse") d.timer = setTimeout(() => lift(d), LONG_PRESS_MS);
  }

  function key(e: KeyboardEvent<HTMLElement>, id: number) {
    if (locked) return;
    if (e.altKey && (e.key === "ArrowUp" || e.key === "ArrowDown")) {
      e.preventDefault();
      moveTo(id, placed.indexOf(id) + (e.key === "ArrowUp" ? -1 : 1));
    } else if ((e.key === "Delete" || e.key === "Backspace") && !e.altKey) {
      e.preventDefault();
      remove(id);
    }
  }

  const reset = () => { commit([]); setActive(null); setAnnounce("Cleared. All lines are back in the list."); };
  const btn = "inline-flex min-h-11 min-w-11 items-center justify-center rounded-lg border border-border bg-surface px-3 text-sm font-medium hover:bg-surface-2 disabled:cursor-not-allowed disabled:opacity-50 aria-disabled:cursor-not-allowed aria-disabled:opacity-50";

  return (
    <div className="space-y-5">
      <div>
        <div className="flex items-center justify-between gap-2">
          <h3 className="text-sm font-semibold">Your program <span className="font-normal text-muted">({placed.length} of {lines.length} lines)</span></h3>
          <button type="button" className={btn} onClick={reset} disabled={locked || placed.length === 0}>Start over</button>
        </div>
        {placed.length === 0 ? (
          <p className="mt-2 rounded-lg border border-dashed border-border p-4 text-sm text-muted">Tap lines below to add them here, then drag the grip, or press and hold a line, to reorder.</p>
        ) : (
          <ol aria-label="Your program, top to bottom" className="mt-2 space-y-2">
            {placed.map((id, i) => {
              const line = byId.get(id)!;
              const isLifted = lifted === id;
              return (
                <li
                  key={id}
                  ref={(el) => { if (el) rows.current.set(id, el); else rows.current.delete(id); }}
                  onPointerDown={(e) => down(e, id)}
                  onContextMenu={(e) => e.preventDefault()}
                  className={`relative select-none rounded-lg border bg-surface [-webkit-touch-callout:none] ${isLifted ? "z-10 border-accent bg-surface-2 shadow-lg" : active === id ? "border-accent" : "border-border"}`}
                >
                  <div className="flex items-stretch">
                    <span data-grip aria-hidden="true" title="Drag to reorder" className="flex min-h-11 w-11 shrink-0 cursor-grab touch-none items-center justify-center text-lg text-muted">⠿</span>
                    <button
                      type="button"
                      data-line
                      aria-label={`Line ${i + 1}: ${line.text.trim() || "blank"}. ${active === id ? "Move controls are shown below." : "Press to show move controls."}`}
                      aria-expanded={active === id}
                      onClick={() => setActive(active === id ? null : id)}
                      onKeyDown={(e) => key(e, id)}
                      className="min-h-11 min-w-0 flex-1 overflow-x-auto rounded-r-lg py-2 pr-3 text-left"
                    >
                      <pre className="font-mono text-[13px] leading-5 whitespace-pre">{highlight(line.text, language, `${id}-`)}</pre>
                    </button>
                  </div>
                  {active === id && !isLifted && (
                    <div data-control className="flex flex-wrap gap-2 border-t border-border p-2">
                      <button type="button" className={btn} aria-disabled={i === 0 || locked} onClick={() => i > 0 && moveTo(id, i - 1)}>Move up</button>
                      <button type="button" className={btn} aria-disabled={i === placed.length - 1 || locked} onClick={() => i < placed.length - 1 && moveTo(id, i + 1)}>Move down</button>
                      <button type="button" className={btn} onClick={() => remove(id)} disabled={locked}>Remove</button>
                    </div>
                  )}
                </li>
              );
            })}
          </ol>
        )}
      </div>

      <div>
        <h3 className="text-sm font-semibold">
          Lines to place <span className="font-normal text-muted">({pool.length} left{practice.hasDistractors ? "; not every line belongs" : ""})</span>
        </h3>
        {pool.length === 0 ? (
          <p className="mt-2 text-sm text-muted">Every line is placed.</p>
        ) : (
          <ul id={`${practice.id}-pool`} className="mt-2 space-y-2">
            {pool.map((l) => (
              <li key={l.id}>
                <button type="button" disabled={locked} onClick={() => add(l.id)} aria-label={`Add line: ${l.text.trim() || "blank"}`} className="flex min-h-11 w-full items-center gap-3 overflow-x-auto rounded-lg border border-border bg-surface-2 px-3 py-2 text-left hover:border-accent disabled:opacity-60">
                  <span aria-hidden="true" className="text-accent">+</span>
                  <pre className="font-mono text-[13px] leading-5 whitespace-pre">{highlight(l.text, language, `p${l.id}-`)}</pre>
                </button>
              </li>
            ))}
          </ul>
        )}
      </div>
      <p role="status" aria-live="polite" className="sr-only">{announce}</p>
    </div>
  );
}
