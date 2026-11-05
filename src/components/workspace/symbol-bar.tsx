"use client";

import { cursorCharLeft, cursorCharRight, cursorLineDown, cursorLineUp, redo, undo } from "@codemirror/commands";
import { hasNextSnippetField, nextSnippetField, snippet } from "@codemirror/autocomplete";
import { EditorSelection } from "@codemirror/state";
import type { EditorView } from "@codemirror/view";
import { type PointerEvent, useEffect, useRef, useState, useSyncExternalStore } from "react";
import type { Language } from "@/lib/curriculum/types";
import { COMMON_SYMBOLS, LANGUAGE_SNIPPETS, LANGUAGE_SYMBOLS } from "./snippets";

const COARSE = "(pointer: coarse)";
const subscribeCoarse = (listener: () => void) => {
  const mq = window.matchMedia(COARSE);
  mq.addEventListener("change", listener);
  return () => mq.removeEventListener("change", listener);
};
/** True on touch-first devices. False during SSR and hydration, so server and client markup agree. */
export const useCoarsePointer = () => useSyncExternalStore(subscribeCoarse, () => window.matchMedia(COARSE).matches, () => false);

/**
 * A physical keyboard is assumed once a keydown looks like one: a modifier chord or a navigation key. Soft keyboards
 * report `Unidentified`/keyCode 229 and never send these. Kept in memory only, so nothing is stored.
 */
export function useHardwareKeyboard() {
  const [hardware, setHardware] = useState(false);
  useEffect(() => {
    const onKey = (e: KeyboardEvent) => {
      if (e.isComposing || e.keyCode === 229 || e.key === "Unidentified") return;
      if ((e.ctrlKey || e.metaKey || e.altKey) && e.key.length === 1 && !e.getModifierState("AltGraph")) setHardware(true);
      else if (/^(Escape|Arrow(Left|Right|Up|Down)|Home|End|PageUp|PageDown|F\d+)$/.test(e.key)) setHardware(true);
    };
    window.addEventListener("keydown", onKey, true);
    return () => window.removeEventListener("keydown", onKey, true);
  }, []);
  return hardware;
}

/** Pixels the on-screen keyboard (or any browser UI) covers at the bottom of the layout viewport. */
function useKeyboardInset(active: boolean) {
  const [inset, setInset] = useState(0);
  useEffect(() => {
    const vv = window.visualViewport;
    if (!active || !vv) return;
    const update = () => setInset(Math.max(0, Math.round(window.innerHeight - vv.offsetTop - vv.height)));
    update();
    vv.addEventListener("resize", update);
    vv.addEventListener("scroll", update);
    return () => { vv.removeEventListener("resize", update); vv.removeEventListener("scroll", update); setInset(0); };
  }, [active]);
  return inset;
}

const key = "inline-flex min-h-11 min-w-11 shrink-0 items-center justify-center rounded-lg border border-border bg-surface px-3 font-mono text-base text-text active:bg-surface-2";
const chip = "inline-flex min-h-11 shrink-0 items-center justify-center rounded-full border border-border bg-bg px-4 text-sm text-text active:bg-surface-2";
const STEP_X = 12;
const STEP_Y = 22;

/**
 * Touch helpers docked above the on-screen keyboard: symbols, snippet chips, undo/redo, a Tab/next-field key and a pad
 * that moves the caret by finger. Pointer-down is cancelled so the editor never loses focus (and the keyboard never closes).
 */
export function SymbolBar({ view, language, tabWidth, onHeight }: { view: EditorView | null; language: Language; tabWidth: number; onHeight?: (px: number) => void }) {
  const inset = useKeyboardInset(true);
  const ref = useRef<HTMLDivElement>(null);
  useEffect(() => {
    const el = ref.current;
    if (!el || !onHeight) return;
    onHeight(el.offsetHeight);
    return () => onHeight(0);
  }, [onHeight]);
  const keep = (e: PointerEvent | React.MouseEvent) => e.preventDefault();

  const insert = (text: string) => {
    if (!view) return;
    view.dispatch(view.state.changeByRange((r) => ({ changes: { from: r.from, to: r.to, insert: text }, range: EditorSelection.cursor(r.from + text.length) })), { scrollIntoView: true, userEvent: "input" });
    view.focus();
  };
  const tab = () => {
    if (!view) return;
    if (hasNextSnippetField(view.state)) { nextSnippetField(view); return; }
    insert(" ".repeat(tabWidth));
  };
  const apply = (template: string) => {
    if (!view) return;
    const { from, to } = view.state.selection.main;
    snippet(template)(view, null, from, to);
    view.focus();
  };

  // Caret pad: dragging moves the caret one character per STEP_X pixels and one line per STEP_Y pixels.
  const drag = useRef<{ x: number; y: number } | null>(null);
  const padDown = (e: PointerEvent<HTMLButtonElement>) => { e.preventDefault(); e.currentTarget.setPointerCapture(e.pointerId); drag.current = { x: e.clientX, y: e.clientY }; };
  const padMove = (e: PointerEvent<HTMLButtonElement>) => {
    const d = drag.current;
    if (!d || !view) return;
    while (e.clientX - d.x >= STEP_X) { cursorCharRight(view); d.x += STEP_X; }
    while (d.x - e.clientX >= STEP_X) { cursorCharLeft(view); d.x -= STEP_X; }
    while (e.clientY - d.y >= STEP_Y) { cursorLineDown(view); d.y += STEP_Y; }
    while (d.y - e.clientY >= STEP_Y) { cursorLineUp(view); d.y -= STEP_Y; }
  };
  const padEnd = () => { drag.current = null; };

  return (
    <div
      ref={ref}
      role="toolbar"
      aria-label="Code keys"
      style={{ bottom: inset }}
      className="fixed inset-x-0 z-40 space-y-1.5 border-t border-border bg-surface-2 px-2 py-1.5 shadow-[0_-2px_8px_rgb(0_0_0/0.18)] pb-[max(0.375rem,env(safe-area-inset-bottom))]"
      onPointerDown={keep}
      onMouseDown={keep}
    >
      <div className="flex gap-1.5 overflow-x-auto" tabIndex={-1}>
        {LANGUAGE_SNIPPETS[language].map((s) => (
          <button key={s.label} type="button" className={chip} onClick={() => apply(s.template)}>{s.label}</button>
        ))}
      </div>
      <div className="flex gap-1.5 overflow-x-auto">
        <button type="button" aria-label="Move the cursor: drag in any direction" title="Drag to move the cursor" className={`${key} touch-none select-none`} onPointerDown={padDown} onPointerMove={padMove} onPointerUp={padEnd} onPointerCancel={padEnd}>✥</button>
        <button type="button" className={key} onClick={tab} aria-label="Tab or next snippet field">Tab</button>
        {[...COMMON_SYMBOLS, ...LANGUAGE_SYMBOLS[language]].map((s) => (
          <button key={s} type="button" className={key} aria-label={`Insert ${s.trim() || "space"}`} onClick={() => insert(s)}>{s}</button>
        ))}
        <button type="button" className={key} aria-label="Undo" onClick={() => view && undo(view)}>↶</button>
        <button type="button" className={key} aria-label="Redo" onClick={() => view && redo(view)}>↷</button>
      </div>
    </div>
  );
}
