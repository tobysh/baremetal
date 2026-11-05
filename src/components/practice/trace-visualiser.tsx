"use client";

import { useMemo, useRef, useState } from "react";
import { highlight } from "./highlight";
import type { KindProps } from "./types";

type Step = { label: string; choices: string[] };

/**
 * Step labels may start with the register or stack slot they fill, in square brackets: "[rax] after `add rax, rsi`".
 * A name starting with "stack" or "mem" is drawn in the stack column, anything else is a register. A backticked
 * snippet in the label highlights the first code line containing it. Without a prefix the step gets its own "step N" cell.
 */
function parseStep(step: Step, i: number) {
  const m = /^\[([^\]]+)\]\s*([\s\S]*)$/.exec(step.label);
  const name = m ? m[1].trim() : `step ${i + 1}`;
  const text = m ? m[2] : step.label;
  const snippet = /`([^`]+)`/.exec(text)?.[1]?.trim();
  return { name, text, snippet, stack: /^(stack|mem)/i.test(name) };
}

/** Step through a short program and fill in what each register or stack slot holds, by tapping a value or dragging it into the cell. */
export function TraceVisualiser({ practice, trace, language, locked, onAnswer }: KindProps<"predict"> & { trace: Step[] }) {
  const steps = useMemo(() => trace.map(parseStep), [trace]);
  const [answers, setAnswers] = useState<(number | null)[]>(() => trace.map(() => null));
  const [cur, setCur] = useState(0);
  const [drag, setDrag] = useState<{ text: string; x: number; y: number; over: boolean } | null>(null);
  const dragRef = useRef<{ id: number; x0: number; y0: number; moved: boolean } | null>(null);
  const justDragged = useRef(false);
  const done = locked;

  const step = steps[cur];
  const lines = practice.code?.replace(/\n$/, "").split("\n") ?? [];
  const hot = step.snippet ? lines.findIndex((l) => l.includes(step.snippet!)) : -1;

  // One cell per distinct name, showing the latest value answered for it up to and including the step being worked on.
  const cells = useMemo(() => {
    const order: string[] = [];
    for (const s of steps) if (!order.includes(s.name)) order.push(s.name);
    return order.map((name) => {
      let value: string | null = null;
      let last = -1;
      steps.forEach((s, i) => {
        if (s.name === name && answers[i] != null && i <= cur) { value = trace[i].choices[answers[i]!]; last = i; }
      });
      return { name, value, last, current: step.name === name, stack: steps.find((s) => s.name === name)!.stack };
    });
  }, [steps, answers, cur, step.name, trace]);
  const regs = cells.filter((c) => !c.stack);
  const stack = cells.filter((c) => c.stack);

  function choose(choice: number) {
    if (locked) return;
    const nextAnswers = answers.map((v, i) => (i === cur ? choice : v));
    setAnswers(nextAnswers);
    onAnswer(nextAnswers.every((v) => v != null) ? (nextAnswers as number[]) : null);
    // Move on to the next unanswered step, if there is one after this.
    const next = answers.findIndex((v, i) => i > cur && v == null);
    if (next >= 0) setCur(next);
    else if (cur < steps.length - 1 && answers[cur + 1] == null) setCur(cur + 1);
  }

  function onPointerDown(e: React.PointerEvent<HTMLButtonElement>, choice: number) {
    if (e.pointerType === "mouse" && e.button !== 0) return;
    dragRef.current = { id: choice, x0: e.clientX, y0: e.clientY, moved: false };
    e.currentTarget.setPointerCapture(e.pointerId);
  }
  function onPointerMove(e: React.PointerEvent<HTMLButtonElement>) {
    const d = dragRef.current;
    if (!d) return;
    if (!d.moved && Math.hypot(e.clientX - d.x0, e.clientY - d.y0) < 8) return;
    d.moved = true;
    const over = !!document.elementFromPoint(e.clientX, e.clientY)?.closest("[data-drop-cell]");
    setDrag({ text: trace[cur].choices[d.id], x: e.clientX, y: e.clientY, over });
  }
  function onPointerUp(e: React.PointerEvent<HTMLButtonElement>) {
    const d = dragRef.current;
    dragRef.current = null;
    setDrag(null);
    if (!d?.moved) return;
    justDragged.current = true;
    setTimeout(() => { justDragged.current = false; }, 0);
    if (document.elementFromPoint(e.clientX, e.clientY)?.closest("[data-drop-cell]")) choose(d.id);
  }

  const filled = answers.filter((v) => v != null).length;
  const cell = (c: (typeof cells)[number]) => (
    <button
      key={c.name}
      type="button"
      data-drop-cell={c.current ? "" : undefined}
      disabled={done}
      onClick={() => { const i = steps.findIndex((s) => s.name === c.name && (c.last < 0 || steps.indexOf(s) === c.last)); if (i >= 0) setCur(i); }}
      aria-current={c.current ? "step" : undefined}
      aria-label={`${c.name}: ${c.value ?? "empty"}${c.current ? ", current step" : ""}`}
      className={`flex min-h-11 flex-col items-start justify-center rounded-lg border px-3 py-1 text-left font-mono text-sm ${c.current ? (drag?.over ? "border-accent bg-accent-soft ring-2 ring-accent" : "border-accent bg-surface ring-1 ring-accent") : "border-line-strong bg-surface"}`}
    >
      <span className="text-[11px] text-muted">{c.name}</span>
      <span className={c.value ? "text-text" : "text-muted"}>{c.value ?? "?"}</span>
    </button>
  );

  return (
    <div>
      <p className="text-sm text-text-2">{practice.prompt}</p>
      {lines.length > 0 && (
        <pre tabIndex={0} aria-label="Program" className="mt-3 overflow-x-auto rounded-xl border border-line-strong bg-[var(--code-bg)] py-2 font-mono text-[13px] text-[var(--code-text)]">
          {lines.map((l, i) => (
            <code key={i} className={`block whitespace-pre px-3 ${i === hot ? "bg-accent-soft ring-1 ring-inset ring-accent" : ""}`}>
              <span aria-hidden="true" className="mr-3 inline-block w-5 select-none text-right text-[var(--code-muted)]">{i + 1}</span>
              {l ? highlight(l, language, `${i}-`) : " "}
              {i === hot && <span className="sr-only"> (current instruction)</span>}
            </code>
          ))}
        </pre>
      )}

      <div className={`mt-3 grid gap-3 ${stack.length ? "sm:grid-cols-[2fr_1fr]" : ""}`}>
        <div>
          <h3 className="text-xs font-semibold uppercase tracking-wide text-muted">Registers and values</h3>
          <div className="mt-1 grid grid-cols-2 gap-2 sm:grid-cols-3">{regs.map(cell)}</div>
        </div>
        {stack.length > 0 && (
          <div>
            <h3 className="text-xs font-semibold uppercase tracking-wide text-muted">Stack (top first)</h3>
            <div className="mt-1 grid gap-2">{stack.map(cell)}</div>
          </div>
        )}
      </div>

      <section aria-label={`Step ${cur + 1} of ${steps.length}`} className="mt-4 rounded-xl border border-line-strong bg-surface p-3">
        <p className="text-xs text-muted">Step {cur + 1} of {steps.length} · {filled} answered</p>
        <p className="mt-1 text-sm"><span className="font-mono font-semibold">{step.name}</span> {step.text.replace(/`/g, "")}</p>
        <div role="group" aria-label="Values for this step. Tap one, or drag it onto the highlighted cell." className="mt-2 flex flex-wrap gap-2">
          {trace[cur].choices.map((c, i) => {
            const on = answers[cur] === i;
            return (
              <button
                key={i}
                type="button"
                aria-pressed={on}
                disabled={done}
                onPointerDown={(e) => onPointerDown(e, i)}
                onPointerMove={onPointerMove}
                onPointerUp={onPointerUp}
                onPointerCancel={() => { dragRef.current = null; setDrag(null); }}
                onClick={() => { if (!justDragged.current) choose(i); }}
                style={{ touchAction: "none" }}
                className={`min-h-11 min-w-11 select-none rounded-lg border px-3 font-mono text-sm ${on ? "border-accent bg-accent-soft ring-1 ring-accent" : "border-line-strong bg-surface-2 hover:bg-panel-2"}`}
              >
                {c}
              </button>
            );
          })}
        </div>
        <div className="mt-3 flex items-center justify-between gap-2">
          <button type="button" onClick={() => setCur((c) => Math.max(0, c - 1))} disabled={cur === 0} className="min-h-11 rounded-lg border border-line-strong px-4 text-sm disabled:opacity-50">Previous</button>
          <button type="button" onClick={() => setCur((c) => Math.min(steps.length - 1, c + 1))} disabled={cur === steps.length - 1} className="min-h-11 rounded-lg border border-line-strong px-4 text-sm disabled:opacity-50">Next step</button>
        </div>
      </section>

      <p className="mt-3 text-xs text-muted">{filled < steps.length ? `${steps.length - filled} more to answer, then check.` : "All steps answered. Check your trace."}</p>

      {drag && (
        <div aria-hidden="true" className="pointer-events-none fixed z-50 -translate-x-1/2 -translate-y-full rounded-lg border border-accent bg-surface px-3 py-2 font-mono text-sm shadow-lg" style={{ left: drag.x, top: drag.y - 8 }}>
          {drag.text}
        </div>
      )}
    </div>
  );
}
