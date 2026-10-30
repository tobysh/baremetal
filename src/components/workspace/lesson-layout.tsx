"use client";

import { useRef, useState, useSyncExternalStore, type CSSProperties, type KeyboardEvent, type PointerEvent, type ReactNode } from "react";
import styles from "./lesson-layout.module.css";

const preferenceKey = "bm:workspace:reading-width";
const subscribe = (listener: () => void) => {
  window.addEventListener("storage", listener);
  return () => window.removeEventListener("storage", listener);
};
function readWidth() {
  try {
    const value = Number(localStorage.getItem(preferenceKey) ?? 46);
    return Number.isFinite(value) ? Math.max(32, Math.min(60, value)) : 46;
  } catch { return 46; }
}

/** Both panes stay mounted so switching views never discards code or tutor state. */
export function LessonLayout({ reading, workspace }: { reading: ReactNode; workspace: ReactNode }) {
  const savedWidth = useSyncExternalStore(subscribe, readWidth, () => 46);
  const [width, setWidth] = useState<number | null>(null);
  const [view, setView] = useState<"lesson" | "code">("lesson");
  const [focused, setFocused] = useState(false);
  const readingWidth = width ?? savedWidth;
  const panes = useRef<HTMLDivElement>(null);
  const [dragging, setDragging] = useState(false);
  const clamp = (value: number) => Math.max(32, Math.min(60, Math.round(value)));

  function resize(value: number) {
    setWidth(value);
    try { localStorage.setItem(preferenceKey, String(value)); } catch { /* Layout still works without storage. */ }
  }

  function drag(event: PointerEvent<HTMLDivElement>) {
    const rect = panes.current?.getBoundingClientRect();
    if (!dragging || !rect?.width) return;
    resize(clamp(((event.clientX - rect.left) / rect.width) * 100));
  }
  function keys(event: KeyboardEvent<HTMLDivElement>) {
    const step = event.shiftKey ? 5 : 2;
    const next = event.key === "ArrowLeft" ? readingWidth - step : event.key === "ArrowRight" ? readingWidth + step : event.key === "Home" ? 32 : event.key === "End" ? 60 : null;
    if (next === null) return;
    event.preventDefault();
    resize(clamp(next));
  }

  return (
    <div className={styles.layout} data-view={view} data-focused={focused} style={{ "--reading-width": `${readingWidth}%` } as CSSProperties}>
      <div className={styles.toolbar}>
        <div className={styles.views} role="tablist" aria-label="Lesson views">
          {(["lesson", "code"] as const).map((item) => (
            <button key={item} id={`view-${item}`} type="button" role="tab" aria-selected={view === item} aria-controls={`view-panel-${item}`} tabIndex={view === item ? 0 : -1}
              onClick={() => setView(item)} onKeyDown={(event) => {
                if (!["ArrowLeft", "ArrowRight", "Home", "End"].includes(event.key)) return;
                event.preventDefault();
                const target = event.key === "Home" ? "lesson" : event.key === "End" ? "code" : view === "lesson" ? "code" : "lesson";
                setView(target);
                document.getElementById(`view-${target}`)?.focus();
              }}>
              {item === "lesson" ? "Read lesson" : "Code & feedback"}
            </button>
          ))}
        </div>
        <div className={styles.desktopControls}>
          <label className="flex items-center gap-3 text-xs text-muted">
            Reading width
            <input aria-label="Reading pane width" aria-valuetext={`${readingWidth} percent`} type="range" min={32} max={60} value={readingWidth} onChange={(event) => resize(Number(event.target.value))} disabled={focused} className="w-24 accent-accent" />
          </label>
          <button type="button" aria-pressed={focused} onClick={() => setFocused(!focused)} className="rounded-lg border border-border px-3 py-2 text-xs font-medium hover:bg-surface-2">
            {focused ? "Show lesson" : "Focus on code"}
          </button>
        </div>
      </div>
      <div className={styles.panes} ref={panes}>
        <div id="view-panel-lesson" role="tabpanel" tabIndex={0} aria-labelledby="view-lesson" className={styles.reading}>{reading}</div>
        <div
          role="separator" aria-orientation="vertical" aria-label="Resize the reading pane" aria-valuemin={32} aria-valuemax={60} aria-valuenow={readingWidth} aria-valuetext={`${readingWidth} percent`}
          tabIndex={0} className={styles.divider} data-dragging={dragging}
          onPointerDown={(event) => { event.currentTarget.setPointerCapture(event.pointerId); setDragging(true); }}
          onPointerMove={drag} onPointerUp={() => setDragging(false)} onPointerCancel={() => setDragging(false)} onKeyDown={keys}
        />
        <div id="view-panel-code" role="tabpanel" tabIndex={0} aria-labelledby="view-code" className={styles.workspace}>{workspace}</div>
      </div>
    </div>
  );
}
