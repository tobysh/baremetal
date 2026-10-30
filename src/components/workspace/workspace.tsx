"use client";

import Link from "next/link";
import { useRouter } from "next/navigation";
import { type CSSProperties, type KeyboardEvent, type PointerEvent, Suspense, lazy, useCallback, useEffect, useLayoutEffect, useMemo, useRef, useState, useSyncExternalStore } from "react";
import { LANGUAGE_FILE, LANGUAGE_LABEL, type PublicExercise } from "@/lib/curriculum/types";
import { track } from "../analytics";
import { MiniMarkdown } from "./mini-markdown";
import { explainFailure, type Explanation } from "@/lib/diagnostics";
import { errorLines } from "@/lib/error-lines";
import { OutputDiff } from "./output-diff";
import { TutorChat, type ChatMessage, type TutorControl } from "./tutor-chat";
import { AchievementToasts, Celebration, type Reward, type TrackComplete, type Unlocked } from "../celebrations";

// The editor is the heaviest dependency on the site: load it only in the
// browser, only on lesson pages, and show the code as plain text until then
// (so the server-rendered HTML contains the starter code too).
const NO_LINES: number[] = [];
const CodeEditor = lazy(() => import("./code-editor"));
const noop = () => () => {};
// Keep per-draft writes ordered even when navigation remounts a workspace.
const saveQueues = new Map<string, Promise<void>>();
const splitKey = "bm:workspace:editor-split";
const clampSplit = (value: number) => Math.max(30, Math.min(75, Math.round(value)));
const subscribeStorage = (listener: () => void) => {
  window.addEventListener("storage", listener);
  return () => window.removeEventListener("storage", listener);
};
function readSplit() {
  try {
    const value = Number(localStorage.getItem(splitKey) ?? 55);
    return Number.isFinite(value) ? clampSplit(value) : 55;
  } catch { return 55; }
}

// Per-viewer preferences (editor font size, tab width, tour dismissal). Every storage access is guarded; an
// in-memory copy keeps the controls working when storage is blocked.
const fontKey = "bm:editor:font-size";
const tabKey = "bm:editor:tab-width";
const tourKey = "bm:workspace:tour-done";
const FONT_MIN = 12, FONT_MAX = 24, FONT_DEFAULT = 15;
const TAB_WIDTHS = [2, 4, 8] as const;
const TAB_DEFAULT = 4;
const memoryPrefs = new Map<string, string>();
const prefListeners = new Set<() => void>();
function readPref(key: string): string | null {
  try { return localStorage.getItem(key) ?? memoryPrefs.get(key) ?? null; } catch { return memoryPrefs.get(key) ?? null; }
}
function writePref(key: string, value: string | null) {
  if (value === null) memoryPrefs.delete(key); else memoryPrefs.set(key, value);
  try { if (value === null) localStorage.removeItem(key); else localStorage.setItem(key, value); } catch { /* keep the in-memory copy */ }
  prefListeners.forEach((listener) => listener());
}
const subscribePrefs = (listener: () => void) => {
  prefListeners.add(listener);
  window.addEventListener("storage", listener);
  return () => { prefListeners.delete(listener); window.removeEventListener("storage", listener); };
};
const readFont = () => {
  const n = Number(readPref(fontKey));
  return Number.isInteger(n) && n >= FONT_MIN && n <= FONT_MAX ? n : FONT_DEFAULT;
};
const readTab = () => {
  const n = Number(readPref(tabKey));
  return (TAB_WIDTHS as readonly number[]).includes(n) ? n : TAB_DEFAULT;
};
const prefersReducedMotion = () => typeof window !== "undefined" && window.matchMedia?.("(prefers-reduced-motion: reduce)").matches;
const formatSeconds = (ms: number) => (ms < 1000 ? `${Math.round(ms)} ms` : `${(ms / 1000).toFixed(1)} s`);

const useIsClient = () => useSyncExternalStore(noop, () => true, () => false);

function readDraft(key: string): string | null {
  try {
    return localStorage.getItem(key);
  } catch {
    return null;
  }
}

// Matches the editor's colours so there's no flash while it loads.
function CodeFallback({ code, fontSize = FONT_DEFAULT, tabWidth = TAB_DEFAULT }: { code: string; fontSize?: number; tabWidth?: number }) {
  return <pre style={{ fontSize, tabSize: tabWidth }} className="h-full overflow-auto bg-[var(--code-bg)] p-4 pl-12 font-mono leading-[1.65] text-[var(--code-text)]">{code}</pre>;
}

const isMac = () => /Mac|iPhone|iPad/.test(navigator.platform || navigator.userAgent);
const useModKey = () => useSyncExternalStore(noop, () => (isMac() ? "⌘" : "Ctrl"), () => "Ctrl");

function Kbd({ children }: { children: string }) {
  return <kbd className="rounded border border-current/30 px-1 font-sans text-[10px] leading-4 opacity-80">{children}</kbd>;
}

type TestCase = { name: string; passed: boolean; message?: string; expected?: string; actual?: string };
type RunResponse =
  | { kind: "run"; ok: boolean; phase: "compile" | "run"; compileOutput: string; stdout: string; stderr: string; exitCode: number | null; error: string | null; durationMs: number }
  | {
      kind: "test";
      compiled: boolean;
      compileOutput: string;
      results: TestCase[];
      allPassed: boolean;
      output: string;
      error: string | null;
      saved: boolean;
      reward: Reward | null;
      paced?: boolean;
      unaided?: boolean;
      reviewed?: boolean;
      achievements?: Unlocked[];
      trackComplete?: TrackComplete | null;
    };

/** "lesson" is the normal exercise; "challenge" is its optional harder twin; "review" re-solves a passed lesson for the schedule (no XP). */
export type Variant = "lesson" | "challenge" | "review";

type Tab = "output" | "hints" | "tutor";

export function Workspace({
  exercise,
  initialCode,
  signedIn,
  draftOwner,
  alreadyPassed,
  tutorHistory,
  nextHref,
  nextLabel,
  loginHref,
  variant = "lesson",
  initialHints = 0,
  onPassed,
}: {
  exercise: PublicExercise;
  initialCode: string | null;
  signedIn: boolean;
  draftOwner?: string;
  alreadyPassed: boolean;
  tutorHistory: ChatMessage[];
  nextHref: string | null;
  nextLabel?: string;
  loginHref: string;
  variant?: Variant;
  /** Hints the server remembers the learner opening, so a reload does not hide them. */
  initialHints?: number;
  onPassed?: () => void;
}) {
  const storageKey = signedIn ? `bm:draft:${draftOwner}:${exercise.id}` : `bm:code:${exercise.id}`;
  // Signed-out visitors keep their code in this browser only.
  const localDraft = useSyncExternalStore(noop, () => (variant === "review" ? null : readDraft(storageKey)), () => null);
  const [edited, setCode] = useState<string | null>(null);
  const code = edited ?? localDraft ?? initialCode ?? exercise.starter;
  const isClient = useIsClient();
  const mod = useModKey();
  const rootRef = useRef<HTMLDivElement>(null);
  const savedSplit = useSyncExternalStore(subscribeStorage, readSplit, () => 55);
  const [dragged, setSplit] = useState<number | null>(null);
  const [expanded, setExpanded] = useState(false);
  const split = expanded ? 30 : (dragged ?? savedSplit);
  const [dragging, setDragging] = useState(false);
  function resizeSplit(value: number) {
    setExpanded(false);
    const next = clampSplit(value);
    setSplit(next);
    try { localStorage.setItem(splitKey, String(next)); } catch { /* ignore */ }
  }
  function dragSplit(event: PointerEvent<HTMLDivElement>) {
    const rect = rootRef.current?.getBoundingClientRect();
    if (!dragging || !rect?.height) return;
    resizeSplit(((event.clientY - rect.top) / rect.height) * 100);
  }
  function splitKeys(event: KeyboardEvent<HTMLDivElement>) {
    const step = event.shiftKey ? 10 : 3;
    const next = event.key === "ArrowUp" ? split - step : event.key === "ArrowDown" ? split + step : event.key === "Home" ? 30 : event.key === "End" ? 75 : null;
    if (next === null) return;
    event.preventDefault();
    resizeSplit(next);
  }
  const fontSize = useSyncExternalStore(subscribePrefs, readFont, () => FONT_DEFAULT);
  const tabWidth = useSyncExternalStore(subscribePrefs, readTab, () => TAB_DEFAULT);
  const tourDone = useSyncExternalStore(subscribePrefs, () => readPref(tourKey) === "1", () => true);
  const [tourStep, setTourStep] = useState(0);
  const showTour = isClient && !tourDone && variant === "lesson" && !alreadyPassed;
  const activeTour = showTour ? TOUR[tourStep]?.id : undefined;
  const ring = (id: string) => (activeTour === id ? " ring-2 ring-accent ring-offset-2 ring-offset-surface" : "");
  const [stdin, setStdin] = useState("");
  const [busy, setBusy] = useState<null | "run" | "test">(null);
  const router = useRouter();
  const [result, setResult] = useState<RunResponse | null>(null);
  const [requestError, setRequestError] = useState<string | null>(null);
  const [passed, setPassed] = useState(alreadyPassed);
  const [tab, setTab] = useState<Tab>("output");
  const [hints, setHints] = useState<string[]>([]);
  const hintsShown = hints.length;
  const [hintBusy, setHintBusy] = useState(false);
  const [hintError, setHintError] = useState<string | null>(null);
  const fetchHints = useCallback(async (index: number) => {
    const res = await fetch("/api/hint", { method: "POST", headers: { "Content-Type": "application/json" }, body: JSON.stringify({ exerciseId: exercise.id, index }) });
    const body = await res.json().catch(() => { throw new Error(`The server could not complete the request (HTTP ${res.status}). Please try again.`); });
    if (!res.ok) throw new Error(body.error ?? "Could not load the hint. Try again.");
    return body.hints as string[];
  }, [exercise.id]);
  useEffect(() => {
    if (variant !== "lesson" || initialHints <= 0) return;
    let cancelled = false;
    fetchHints(Math.min(initialHints, exercise.hintCount) - 1).then((loaded) => {
      if (!cancelled) setHints((current) => current.length > loaded.length ? current : loaded);
    }).catch(() => {});
    return () => { cancelled = true; };
  }, [fetchHints, initialHints, exercise.hintCount, variant]);
  const [toasts, setToasts] = useState<Unlocked[]>([]);
  const [celebration, setCelebration] = useState<{ levelUp: Reward | null; trackComplete: TrackComplete | null; returnFocus: HTMLElement | null } | null>(null);
  const dismissToast = useCallback((id: string) => setToasts((t) => t.filter((x) => x.id !== id)), []);
  const closeCelebration = useCallback(() => setCelebration(null), []);
  const celebrationFocus = useCallback(() => {
    const previous = celebration?.returnFocus;
    return previous?.isConnected ? previous : rootRef.current?.querySelector<HTMLElement>("[data-primary-action]") ?? null;
  }, [celebration]);
  const [saveState, setSaveState] = useState<"idle" | "saving" | "saved" | "error">("idle");
  const [cancelled, setCancelled] = useState(false);
  const [savedAt, setSavedAt] = useState<number | null>(null);
  const [elapsedMs, setElapsedMs] = useState<number | null>(null);
  const [passedCode, setPassedCode] = useState<string | null>(null);
  // Reset keeps the previous code for a few seconds instead of asking first (the editor's own undo history can't restore it).
  const [undoReset, setUndoReset] = useState<string | null>(null);
  const tutorRef = useRef<TutorControl>(null);
  const saveTimer = useRef<ReturnType<typeof setTimeout> | null>(null);
  const dirty = useRef(false);
  const revision = useRef(0);
  const alive = useRef(true);
  const execution = useRef<AbortController | null>(null);
  const latestDraft = useRef<string | null>(null);

  const saveDraft = useCallback((value: string, version: number) => {
    // Serialize writes: aborting a fetch cannot undo a write already accepted by the server.
    const task = (saveQueues.get(storageKey) ?? Promise.resolve()).then(async () => {
      if (version !== revision.current) return;
      if (alive.current) setSaveState("saving");
      try {
        if (signedIn) {
          const body = JSON.stringify({ exerciseId: exercise.id, code: value });
          const res = await fetch("/api/progress", {
            method: "PUT",
            // Browser keepalive requests share a 64 KiB budget. Large drafts use
            // a normal request and retain the recovery copy if navigation interrupts it.
            keepalive: !alive.current && new TextEncoder().encode(body).byteLength <= 60 * 1024,
            headers: { "Content-Type": "application/json" },
            body,
          });
          if (!res.ok) throw new Error("Save failed");
        } else {
          localStorage.setItem(storageKey, value);
        }
        if (version !== revision.current) return;
        dirty.current = false;
        if (signedIn) {
          try { if (localStorage.getItem(storageKey) === value) localStorage.removeItem(storageKey); }
          catch { /* A successful account save must not depend on browser storage. */ }
        }
        if (alive.current) { setSaveState("saved"); setSavedAt(Date.now()); }
      } catch {
        if (alive.current && version === revision.current) setSaveState("error");
      }
    });
    saveQueues.set(storageKey, task);
    void task.finally(() => { if (saveQueues.get(storageKey) === task) saveQueues.delete(storageKey); });
    return task;
  }, [exercise.id, signedIn, storageKey]);

  useEffect(() => {
    alive.current = true;
    const warn = (event: BeforeUnloadEvent) => {
      if (!dirty.current) return;
      event.preventDefault();
      event.returnValue = "";
    };
    window.addEventListener("beforeunload", warn);
    const version = revision; // This is a draft counter, intentionally read at cleanup.
    return () => {
      alive.current = false;
      execution.current?.abort();
      if (saveTimer.current) clearTimeout(saveTimer.current);
      if (dirty.current && latestDraft.current !== null && variant !== "review") void saveDraft(latestDraft.current, version.current);
      window.removeEventListener("beforeunload", warn);
    };
  }, [saveDraft, variant]);

  const onChange = useCallback((value: string) => {
    setCode(value);
    if (variant === "review") return;
    dirty.current = true;
    latestDraft.current = value;
    const version = ++revision.current;
    // Keep an account-scoped recovery copy until the latest server save succeeds.
    try { localStorage.setItem(storageKey, value); } catch { /* Report failure through the save attempt. */ }
    setSaveState("idle");
    if (saveTimer.current) clearTimeout(saveTimer.current);
    saveTimer.current = setTimeout(() => void saveDraft(value, version), 1200);
  }, [saveDraft, storageKey, variant]);

  const retrySave = () => {
    if (saveTimer.current) clearTimeout(saveTimer.current);
    dirty.current = true;
    latestDraft.current = code;
    void saveDraft(code, ++revision.current);
  };
  const recovered = signedIn && variant !== "review" && edited === null && localDraft !== null && saveState === "idle";

  // Latest values for callbacks handed to the editor's keymap.
  const codeRef = useRef(code);
  const stdinRef = useRef(stdin);
  useLayoutEffect(() => {
    codeRef.current = code;
    stdinRef.current = stdin;
  }, [code, stdin]);

  const execute = useCallback(
    async (mode: "run" | "test") => {
      if (execution.current) return; // Keyboard shortcuts obey the same admission rule as buttons.
      const returnFocus = document.activeElement as HTMLElement | null;
      const controller = new AbortController();
      execution.current = controller;
      setBusy(mode);
      setRequestError(null);
      setCancelled(false);
      setElapsedMs(null);
      setTab("output");
      const startedAt = performance.now();
      const sentCode = codeRef.current;
      track("run");
      try {
        if (saveTimer.current) clearTimeout(saveTimer.current);
        if (dirty.current && variant !== "review") await saveDraft(codeRef.current, revision.current);
        controller.signal.throwIfAborted();
        const res = await fetch("/api/run", {
          method: "POST",
          signal: controller.signal,
          headers: { "Content-Type": "application/json" },
          body: JSON.stringify({ exerciseId: exercise.id, code: codeRef.current, stdin: stdinRef.current, mode, review: variant === "review" }),
        });
        const body = await res.json().catch(() => { throw new Error(`The server could not run your code (HTTP ${res.status}). Please try again.`); });
        if (!res.ok) throw new Error(res.status === 503 ? "The code runner is unavailable or busy. Your draft is still here; try again shortly." : body.error ?? "Something went wrong running your code.");
        if (!alive.current || controller.signal.aborted) return;
        setResult(body);
        setElapsedMs(performance.now() - startedAt);
        if (body.kind === "test" && body.allPassed) {
          setPassed(true);
          setPassedCode(sentCode);
          writePref(tourKey, "1"); // the tour is for the very first lessons only
          track("test_passed");
          if (body.reward || body.reviewed) router.refresh(); // header XP chip and dashboard read the user at request time
          if (variant === "lesson") onPassed?.();
          if (body.achievements?.length) setToasts((t) => [...t, ...body.achievements]);
          const levelUp = body.reward?.levelUp ? body.reward : null;
          if (levelUp || body.trackComplete) setCelebration({ levelUp, trackComplete: body.trackComplete ?? null, returnFocus });
        }
      } catch (e) {
        if (alive.current && !controller.signal.aborted) setRequestError((e as Error).message);
      } finally {
        if (execution.current === controller) execution.current = null;
        if (alive.current) setBusy(null);
      }
    },
    [exercise.id, router, variant, onPassed, saveDraft],
  );
  const cancelRun = useCallback(() => {
    if (!execution.current) return;
    execution.current.abort(); // the server cancels the job when the request is aborted
    setCancelled(true);
  }, []);
  const resetCode = () => {
    setUndoReset(codeRef.current);
    onChange(exercise.starter);
  };
  const openHint = async () => {
    if (hintBusy || hintsShown >= exercise.hintCount) return;
    setHintBusy(true);
    setHintError(null);
    try {
      const loaded = await fetchHints(hintsShown);
      setHints((current) => current.length > loaded.length ? current : loaded);
      track("hint_open");
    } catch (error) {
      setHintError((error as Error).message);
    } finally {
      setHintBusy(false);
    }
  };
  // After a pass (and until the code is edited) the primary action is the next lesson.
  const ready = passed && !!nextHref && variant === "lesson" && (passedCode !== null ? code === passedCode : edited === null);
  const readyRef = useRef(false);
  const nextRef = useRef<string | null>(nextHref);
  useLayoutEffect(() => {
    readyRef.current = ready;
    nextRef.current = nextHref;
  }, [ready, nextHref]);
  const run = useCallback(() => execute("run"), [execute]);
  const test = useCallback(() => {
    if (readyRef.current && nextRef.current) router.push(nextRef.current);
    else void execute("test");
  }, [execute, router]);
  const retest = useCallback(() => execute("test"), [execute]);

  // Bring the first failing test into view once results arrive.
  useEffect(() => {
    if (!result || busy || result.kind !== "test" || result.allPassed) return;
    document.getElementById("first-failing-test")?.scrollIntoView({ block: "nearest", behavior: prefersReducedMotion() ? "auto" : "smooth" });
  }, [result, busy]);

  const errorLinesOf = useMemo(() => {
    if (!result || busy) return NO_LINES;
    const compileFailed = result.kind === "run" ? result.phase === "compile" && !result.ok : !result.compiled;
    return errorLines(exercise.language, result.compileOutput, result.kind === "run" ? result.stderr : result.output, compileFailed);
  }, [result, busy, exercise.language]);
  const lastOutput = describeResult(result);
  // One small live region announces the outcome; the big output panel and tutor list are not live themselves.
  const announcement = cancelled ? "Cancelled." : requestError ? requestError : busy ? "" : summarize(result);
  const getContext = useCallback(() => ({ code: codeRef.current, lastOutput }), [lastOutput]);

  const askTutor = (text: string) => {
    setTab("tutor");
    if (signedIn) tutorRef.current?.send(text);
  };

  const tabs: { id: Tab; label: string }[] = [
    { id: "output", label: "Output" },
    { id: "hints", label: `Hints (${hintsShown}/${exercise.hintCount})` },
    { id: "tutor", label: "AI tutor" },
  ];

  // Shortcuts also work outside the editor (e.g. in the input box).
  const onKeyDown = (e: KeyboardEvent) => {
    if (e.key !== "Enter" || !(e.metaKey || e.ctrlKey) || e.defaultPrevented) return;
    e.preventDefault();
    if (e.shiftKey) run();
    else test();
  };

  return (
    <div ref={rootRef} onKeyDown={onKeyDown} className="flex flex-col overflow-clip panel lg:min-h-0 lg:flex-1">
      <div className="sticky top-16 z-10 flex shrink-0 flex-wrap items-center gap-2 border-b border-border bg-surface px-3 py-3 lg:static">
        <span className="font-mono text-xs text-muted">{LANGUAGE_FILE[exercise.language]}</span>
        <span className="text-xs text-muted" aria-live="polite">
          {variant === "review" ? "Review draft: saved solution unchanged" : recovered ? "Recovered browser draft. Save it to your account." : saveState === "error" ? "Couldn't save. Your draft is still in the editor." : signedIn ? (saveState === "saved" && savedAt !== null ? <SavedAgo key={savedAt} at={savedAt} /> : saveState === "saving" ? "Saving…" : edited !== null ? "Unsaved changes" : "") : saveState === "saved" && savedAt !== null ? <SavedAgo key={savedAt} at={savedAt} where=" in this browser" /> : "Saved in this browser only"}
        </span>
        {(saveState === "error" || recovered) && <button type="button" onClick={retrySave} className="min-h-11 rounded-md px-2 text-xs text-accent underline underline-offset-2">{recovered ? "Save recovered draft" : "Retry save"}</button>}
        <div className="ml-auto flex flex-wrap items-center justify-end gap-2">
          <button type="button" title="Replace your code with the starter. You can undo it for a few seconds." className="min-h-11 rounded-md border border-border px-2.5 text-xs text-muted hover:bg-surface-2 hover:text-text sm:min-h-9" onClick={resetCode}>
            Reset code
          </button>
          {/* Rendered right after Reset so the next Tab reaches Undo, though it is positioned over the page. */}
          {undoReset !== null && <UndoToast onUndo={() => { onChange(undoReset); setUndoReset(null); }} onDismiss={() => setUndoReset(null)} />}
          {busy && (
            <button type="button" onClick={cancelRun} className="inline-flex h-11 items-center rounded-lg border border-border px-3 text-sm font-medium hover:bg-surface-2 sm:h-9">
              Cancel
            </button>
          )}
          <button
            type="button"
            onClick={run}
            disabled={!!busy}
            title={`Compile and run your own main() (${mod}+Shift+Enter)`}
            aria-keyshortcuts={mod === "⌘" ? "Meta+Shift+Enter" : "Control+Shift+Enter"}
            className={`inline-flex h-11 items-center gap-2 rounded-lg border border-border px-3 text-sm font-medium hover:bg-surface-2 disabled:opacity-60 sm:h-9${ring("run")}`}
          >
            {busy === "run" ? "Running…" : "Run"}
            <span className="inline-flex items-center gap-0.5 max-sm:hidden">
              <Kbd>{mod}</Kbd>
              <Kbd>⇧</Kbd>
              <Kbd>↵</Kbd>
            </span>
          </button>
          {ready ? (
            <>
              <button
                type="button"
                onClick={retest}
                disabled={!!busy}
                title="Run the tests again"
                className="inline-flex h-11 items-center rounded-lg border border-border px-3 text-sm font-medium hover:bg-surface-2 disabled:opacity-60 sm:h-9"
              >
                {busy === "test" ? "Testing…" : "Re-test"}
              </button>
              <Link
                data-primary-action
                href={nextHref!}
                title={`Go to the next lesson (${mod}+Enter)`}
                aria-keyshortcuts={mod === "⌘" ? "Meta+Enter" : "Control+Enter"}
                className="inline-flex h-11 items-center gap-2 rounded-lg bg-accent px-3 text-sm font-medium text-accent-ink hover:brightness-110 sm:h-9"
              >
                {nextLabel ?? "Next lesson →"}
                <span className="inline-flex items-center gap-0.5 max-sm:hidden">
                  <Kbd>{mod}</Kbd>
                  <Kbd>↵</Kbd>
                </span>
              </Link>
            </>
          ) : (
            <button
              data-primary-action
              type="button"
              onClick={test}
              disabled={!!busy}
              title={`Check your solution against the tests (${mod}+Enter)`}
              aria-keyshortcuts={mod === "⌘" ? "Meta+Enter" : "Control+Enter"}
              className={`inline-flex h-11 items-center gap-2 rounded-lg bg-accent px-3 text-sm font-medium text-accent-ink hover:brightness-110 disabled:opacity-60 sm:h-9${ring("test")}`}
            >
              {busy === "test" ? "Testing…" : "Test"}
              <span className="inline-flex items-center gap-0.5 max-sm:hidden">
                <Kbd>{mod}</Kbd>
                <Kbd>↵</Kbd>
              </span>
            </button>
          )}
        </div>
      </div>

      {showTour && (
        <Tour
          step={tourStep}
          mod={mod}
          onStep={setTourStep}
          onClose={() => writePref(tourKey, "1")}
        />
      )}
      <div className="h-[22rem] min-h-0 lg:h-auto lg:min-h-[8rem] lg:shrink lg:grow-0 lg:basis-(--split)" style={{ "--split": `${split}%` } as CSSProperties}>
        {isClient ? (
          <Suspense fallback={<CodeFallback code={code} fontSize={fontSize} tabWidth={tabWidth} />}>
            <CodeEditor value={code} onChange={onChange} language={exercise.language} onRun={run} onTest={test} label={`${LANGUAGE_LABEL[exercise.language]} code editor`} errorLines={errorLinesOf} fontSize={fontSize} tabWidth={tabWidth} />
          </Suspense>
        ) : (
          <CodeFallback code={code} fontSize={fontSize} tabWidth={tabWidth} />
        )}
      </div>

      <div className="flex shrink-0 flex-wrap items-center gap-x-4 gap-y-1 border-t border-border px-3 py-1 text-[11px] text-muted">
        <p className="hidden sm:block">
          <Kbd>{mod}</Kbd>+<Kbd>↵</Kbd> {ready ? "Next lesson" : "Test"} · <Kbd>{mod}</Kbd>+<Kbd>⇧</Kbd>+<Kbd>↵</Kbd> Run · <Kbd>Tab</Kbd> indent · <Kbd>{mod}</Kbd>+<Kbd>/</Kbd> comment ·{" "}
          <Kbd>Esc</Kbd> then <Kbd>Tab</Kbd> leave editor · <Kbd>Alt</Kbd>+<Kbd>1</Kbd> lesson · <Kbd>Alt</Kbd>+<Kbd>2</Kbd> editor
        </p>
        <div role="group" aria-label="Editor settings" className="ml-auto flex flex-wrap items-center gap-2">
          <span className="flex items-center gap-1">
            Text
            <button type="button" aria-label="Smaller text" disabled={fontSize <= FONT_MIN} onClick={() => writePref(fontKey, String(fontSize - 1))} className="min-h-8 min-w-8 rounded-md border border-border text-xs hover:bg-surface-2 hover:text-text disabled:opacity-50">A−</button>
            <span className="min-w-8 text-center tabular-nums" aria-label={`Font size ${fontSize} pixels`}>{fontSize}px</span>
            <button type="button" aria-label="Larger text" disabled={fontSize >= FONT_MAX} onClick={() => writePref(fontKey, String(fontSize + 1))} className="min-h-8 min-w-8 rounded-md border border-border text-xs hover:bg-surface-2 hover:text-text disabled:opacity-50">A+</button>
          </span>
          <label className="flex items-center gap-1">
            Tab width
            <select value={tabWidth} onChange={(e) => writePref(tabKey, e.target.value)} className="min-h-8 rounded-md border border-field bg-bg px-1 text-text">
              {TAB_WIDTHS.map((w) => <option key={w} value={w}>{w}</option>)}
            </select>
          </label>
          {(fontSize !== FONT_DEFAULT || tabWidth !== TAB_DEFAULT) && (
            <button type="button" onClick={() => { writePref(fontKey, null); writePref(tabKey, null); }} className="min-h-8 rounded-md px-2 hover:bg-surface-2 hover:text-text">Default view</button>
          )}
        </div>
      </div>
      <div
        role="separator" aria-orientation="horizontal" aria-label="Resize the editor and the panel below it" aria-valuemin={30} aria-valuemax={75} aria-valuenow={split}
        tabIndex={0} data-dragging={dragging}
        className="hidden h-2 shrink-0 cursor-row-resize touch-none border-y border-border bg-surface-2 hover:bg-accent-soft focus-visible:outline-2 focus-visible:outline-accent data-[dragging=true]:bg-accent-soft lg:block"
        onPointerDown={(event) => { event.currentTarget.setPointerCapture(event.pointerId); setDragging(true); }}
        onPointerMove={dragSplit} onPointerUp={() => setDragging(false)} onPointerCancel={() => setDragging(false)} onKeyDown={splitKeys}
      />
      <div className="max-lg:border-t max-lg:border-border lg:flex lg:min-h-[8rem] lg:flex-1 lg:flex-col">
      <div role="tablist" aria-label="Workspace panels" className="flex border-b border-border px-2">
        {tabs.map((t) => (
          <button
            key={t.id}
            role="tab"
            id={`tab-${t.id}`}
            aria-selected={tab === t.id}
            aria-controls={`panel-${t.id}`}
            tabIndex={tab === t.id ? 0 : -1}
            onClick={() => setTab(t.id)}
            onKeyDown={(event) => {
              if (!["ArrowLeft", "ArrowRight", "Home", "End"].includes(event.key)) return;
              event.preventDefault();
              const index = tabs.findIndex((item) => item.id === t.id);
              const next = event.key === "Home" ? tabs[0] : event.key === "End" ? tabs[tabs.length - 1] : tabs[(index + (event.key === "ArrowRight" ? 1 : tabs.length - 1)) % tabs.length];
              setTab(next.id);
              document.getElementById(`tab-${next.id}`)?.focus();
            }}
            className={`-mb-px min-h-11 border-b-2 px-3 py-2 text-sm${t.id === "output" ? "" : ring(t.id)} ${tab === t.id ? "border-accent text-text" : "border-transparent text-muted hover:text-text"}`}
          >
            {t.label}
          </button>
        ))}
        <button type="button" aria-pressed={expanded} onClick={() => setExpanded(!expanded)} className="my-auto ml-auto hidden min-h-9 rounded-md px-3 text-xs text-muted hover:bg-surface-2 hover:text-text lg:block">
          {expanded ? "Restore editor" : "Expand panel"}
        </button>
      </div>

      <div className="h-[20rem] min-h-0 lg:h-auto lg:flex-1">
        <div role="tabpanel" id="panel-output" aria-labelledby="tab-output" tabIndex={0} hidden={tab !== "output"} className="h-full overflow-y-auto p-4">
          {exercise.mode === "stdout" && (
            <div className="mb-4">
              <label htmlFor="stdin" className="text-xs text-muted">
                Program input (stdin), used by Run
              </label>
              <textarea
                id="stdin"
                rows={1}
                value={stdin}
                onChange={(e) => setStdin(e.target.value)}
                className="mt-1 block w-full resize-y rounded-md border border-field bg-bg px-2 py-1 font-mono text-sm"
                placeholder="e.g. 8 3"
              />
            </div>
          )}
          {result && !busy && <CopyOutput key={lastOutput} text={lastOutput} meta={elapsedMs !== null ? `${result.kind === "test" ? "Tests" : "Run"} finished in ${formatSeconds(elapsedMs)}` : ""} />}
          <OutputPanel
            result={result}
            busy={busy}
            requestError={requestError}
            cancelled={cancelled}
            passed={passed}
            nextHref={nextHref} nextLabel={nextLabel}
            signedIn={signedIn}
            onAsk={askTutor}
            variant={variant}
            explain={(r) =>
              explainFailure({
                language: exercise.language,
                mode: exercise.mode,
                code,
                starter: exercise.starter,
                compileFailed: r.kind === "run" ? r.phase === "compile" && !r.ok : !r.compiled,
                compileOutput: r.compileOutput,
                error: r.error,
                results: r.kind === "test" ? r.results : [],
                stderr: r.kind === "run" ? r.stderr : r.output,
                runtimeFailed: r.kind === "run" && !r.ok && r.phase === "run",
              })
            }
          />
        </div>
        <div role="tabpanel" id="panel-hints" aria-labelledby="tab-hints" tabIndex={0} hidden={tab !== "hints"} className="h-full overflow-y-auto p-4">
          {hintError && <p role="alert" className="mb-3 text-sm text-err">{hintError}</p>}
          <ol className="space-y-3">
            {hints.map((h, i) => (
              <li key={i} className="rounded-lg border border-border bg-bg p-3 text-sm">
                <p className="mb-1 text-xs font-medium text-muted">Hint {i + 1}</p>
                <MiniMarkdown text={h} />
              </li>
            ))}
          </ol>
          {hintsShown < exercise.hintCount ? (
            <button
              type="button"
              disabled={hintBusy}
              className="mt-3 min-h-11 rounded-lg border border-border px-3 text-sm hover:bg-surface-2 disabled:opacity-50 sm:min-h-9"
              onClick={() => {
                openHint();
              }}
            >
              {hintBusy ? "Loading hint…" : hintsShown === 0 ? "Show a hint" : "Show the next hint"}
            </button>
          ) : (
            <p className="mt-3 text-sm text-muted">
              That&apos;s all the hints. Still stuck?{" "}
              <button type="button" className="text-accent underline underline-offset-2" onClick={() => setTab("tutor")}>
                Ask the tutor
              </button>
              .
            </p>
          )}
        </div>
        <div role="tabpanel" id="panel-tutor" aria-labelledby="tab-tutor" hidden={tab !== "tutor"} className="h-full">
          <TutorChat exerciseId={exercise.id} signedIn={signedIn} initialMessages={tutorHistory} getContext={getContext}
            controlRef={tutorRef}
            loginHref={loginHref}
            onShowHint={
              hintsShown < exercise.hintCount
                ? () => {
                    // Authored hints cost nothing — offer them before a model call.
                    openHint();
                    setTab("hints");
                  }
                : null
            }
          />
        </div>
      </div>
      </div>
      <AchievementToasts items={toasts} onDismiss={dismissToast} />
      <div role="status" aria-live="polite" className="sr-only">{announcement}</div>
      {celebration && <Celebration returnFocus={celebrationFocus} nextLesson={nextHref ? { href: nextHref, title: nextLabel ?? "Next lesson" } : null} levelUp={celebration.levelUp} trackComplete={celebration.trackComplete} onClose={closeCelebration} />}
    </div>
  );
}

function CopyOutput({ text, meta }: { text: string; meta: string }) {
  const [status, setStatus] = useState("Copy output");
  useEffect(() => {
    if (status === "Copy output") return;
    const t = setTimeout(() => setStatus("Copy output"), 2500);
    return () => clearTimeout(t);
  }, [status]);
  return (
    <div className="mb-3 flex items-center justify-between gap-3">
      <p className="text-xs text-muted">{meta}</p>
      <button type="button" className="min-h-9 rounded-lg border border-border px-3 text-xs text-muted hover:bg-surface-2 hover:text-text" onClick={async () => {
        try { await navigator.clipboard.writeText(text); setStatus("Copied"); }
        catch { setStatus("Copy unavailable"); }
      }}><span aria-live="polite">{status}</span></button>
    </div>
  );
}

/** Elapsed timer while a request is in flight. The API answers once, so the phase switch is an estimate. */
function RunProgress({ mode }: { mode: "run" | "test" }) {
  const [ms, setMs] = useState(0);
  useEffect(() => {
    const start = performance.now();
    const id = setInterval(() => setMs(performance.now() - start), 100);
    return () => clearInterval(id);
  }, []);
  const phase = ms < 1500 ? "Compiling…" : mode === "test" ? "Running the tests…" : "Running…";
  return (
    <div className="space-y-2">
      <p className="text-sm text-muted">
        {phase} <span className="tabular-nums" aria-hidden="true">{(ms / 1000).toFixed(1)} s</span>
      </p>
      <div className="skeleton h-4 w-2/3" />
      <div className="skeleton h-4 w-1/2" />
    </div>
  );
}

function OutputPanel({
  result,
  busy,
  requestError,
  cancelled,
  passed,
  nextHref,
  nextLabel,
  signedIn,
  onAsk,
  explain,
  variant,
}: {
  result: RunResponse | null;
  busy: null | "run" | "test";
  requestError: string | null;
  cancelled: boolean;
  passed: boolean;
  nextHref: string | null;
  nextLabel?: string;
  signedIn: boolean;
  onAsk: (text: string) => void;
  explain: (r: RunResponse) => Explanation[];
  variant: Variant;
}) {
  if (busy) {
    return <RunProgress mode={busy} />;
  }
  if (cancelled) return <p className="text-sm text-muted">Cancelled.</p>;
  if (requestError) {
    return (
      <p role="alert" className="rounded-lg bg-err-soft p-3 text-sm text-err">
        {requestError}
      </p>
    );
  }
  if (!result) {
    return (
      <div className="text-sm text-muted">
        <p>
          Press <strong className="text-text">Test</strong> to check your solution, or <strong className="text-text">Run</strong> to run your own <code className="font-mono">main</code>.
        </p>
        {passed && <SuccessBanner nextHref={nextHref} nextLabel={nextLabel} signedIn={signedIn} again variant={variant} />}
      </div>
    );
  }

  const compileFailed = result.kind === "run" ? result.phase === "compile" && !result.ok : !result.compiled;
  const failed = compileFailed || !!result.error || (result.kind === "run" ? !result.ok : !result.allPassed);
  const explanations = failed ? explain(result) : [];
  // With failures, show those first and fold the passing tests away.
  const failingFirst = result.kind === "test" && result.results.some((r) => !r.passed) && result.results.some((r) => r.passed);
  return (
    <div className="space-y-4">
      {result.kind === "test" && result.allPassed && <SuccessBanner nextHref={nextHref} nextLabel={nextLabel} signedIn={signedIn} reward={result.reward} paced={result.paced} unaided={result.unaided} reviewed={result.reviewed} variant={variant} />}

      {explanations.length > 0 && (
        <section aria-labelledby="what-went-wrong" className="space-y-2">
          <h3 id="what-went-wrong" className="text-xs font-medium text-muted">
            What probably went wrong
          </h3>
          {explanations.map((x) => (
            <div key={x.id} className="rounded-lg border border-border bg-bg p-3">
              <p className="text-sm font-medium">{x.title}</p>
              <MiniMarkdown text={x.body} />
              <p className="mt-2 text-sm text-text-2"><strong>Try next:</strong> {x.nextStep}</p>
              <button
                type="button"
                onClick={() => onAsk(`The checker said: "${x.title}". Can you look at my code and explain what I did wrong here, without giving me the full answer?`)}
                className="mt-2 inline-flex min-h-11 items-center rounded-md border border-border px-2.5 text-xs font-medium hover:bg-surface-2 sm:min-h-8"
              >
                {signedIn ? "Elaborate with the AI tutor" : "Elaborate with the AI tutor (free account)"}
              </button>
            </div>
          ))}
        </section>
      )}

      {result.compileOutput && (
        <section>
          <h3 className={`text-xs font-medium ${compileFailed ? "text-err" : "text-muted"}`}>{compileFailed ? "Compiler errors" : "Compiler warnings"}</h3>
          <pre className="mt-1 max-h-60 overflow-auto rounded-lg border border-border bg-bg p-3 font-mono text-xs leading-5 whitespace-pre-wrap">{result.compileOutput}</pre>
        </section>
      )}

      {result.error && (
        <div role="alert" className="rounded-lg bg-err-soft p-3 text-sm text-err">
          {result.error}
        </div>
      )}

      {result.kind === "test" && result.results.length > 0 && (
        <section>
          <h3 className="text-xs font-medium text-muted">
            Tests: {result.results.filter((r) => r.passed).length} of {result.results.length} passing
          </h3>
          <ul className="mt-2 space-y-1.5">
            {(failingFirst ? result.results.filter((r) => !r.passed) : result.results).map((r, i, shown) => <TestItem key={i} r={r} first={!r.passed && shown.findIndex((x) => !x.passed) === i} />)}
          </ul>
          {failingFirst && (
            <details className="mt-3">
              <summary className="cursor-pointer text-sm text-ok">{result.results.filter((r) => r.passed).length} passed ✓</summary>
              <ul className="mt-2 space-y-1.5">
                {result.results.filter((r) => r.passed).map((r, i) => <TestItem key={i} r={r} />)}
              </ul>
            </details>
          )}
        </section>
      )}

      {result.kind === "run" && result.phase === "run" && (
        <section>
          <h3 className="text-xs font-medium text-muted">
            Program output {result.exitCode !== null && <span>(exit code {result.exitCode})</span>}
          </h3>
          <pre className="mt-1 max-h-72 overflow-auto rounded-lg border border-border bg-bg p-3 font-mono text-sm whitespace-pre-wrap">
            {result.stdout || <span className="text-muted">(no output)</span>}
            {result.stderr && <span className="text-err">{`\n${result.stderr}`}</span>}
          </pre>
        </section>
      )}

      {result.kind === "test" && result.output && (
        <section>
          <h3 className="text-xs font-medium text-muted">Other output from your code</h3>
          <pre className="mt-1 max-h-40 overflow-auto rounded-lg border border-border bg-bg p-3 font-mono text-xs whitespace-pre-wrap">{result.output}</pre>
        </section>
      )}

    </div>
  );
}

type TestRow = Extract<RunResponse, { kind: "test" }>["results"][number];
function TestItem({ r, first = false }: { r: TestRow; first?: boolean }) {
  return (
    <li id={first ? "first-failing-test" : undefined} className="scroll-mt-4 text-sm">
      <span className={r.passed ? "text-ok" : "text-err"}>
        <span aria-hidden="true">{r.passed ? "✓" : "✗"}</span>
        <span className="sr-only">{r.passed ? "Passed:" : "Failed:"}</span> {r.name}
      </span>
      {r.expected !== undefined && r.actual !== undefined ? (
        <OutputDiff expected={r.expected} actual={r.actual} />
      ) : r.message && <pre className="mt-1 ml-5 overflow-auto rounded-md bg-bg p-2 font-mono text-xs whitespace-pre-wrap text-muted">{r.message}</pre>}
    </li>
  );
}

function SavedAgo({ at, where = "" }: { at: number; where?: string }) {
  const [now, setNow] = useState(() => Date.now());
  useEffect(() => {
    const id = setInterval(() => setNow(Date.now()), 1000);
    return () => clearInterval(id);
  }, []);
  const s = Math.max(0, Math.floor((now - at) / 1000));
  const ago = s < 60 ? `${s}s ago` : `${Math.floor(s / 60)}m ago`;
  // Only the stable part is announced; the ticking age is visual.
  return <><span>Saved{where}</span><span aria-hidden="true"> · {ago}</span></>;
}

const TOUR = [
  { id: "run", title: "Run", body: (mod: string) => `Compiles and runs your own main(), so you can try things out. Shortcut: ${mod}+Shift+Enter.` },
  { id: "test", title: "Test", body: (mod: string) => `Checks your solution against the hidden tests and saves your progress when they pass. Shortcut: ${mod}+Enter.` },
  { id: "hints", title: "Hints", body: () => "Stuck? Open the Hints tab. They appear one at a time and cost nothing." },
  { id: "tutor", title: "AI tutor", body: () => "Ask the tutor in the AI tutor tab. It explains and nudges, but never hands over the answer." },
];

function Tour({ step, mod, onStep, onClose }: { step: number; mod: string; onStep: (n: number) => void; onClose: () => void }) {
  const item = TOUR[step];
  const last = step === TOUR.length - 1;
  return (
    <section aria-label="Quick tour" className="shrink-0 border-b border-border bg-accent-soft px-3 py-2 text-sm">
      <div className="flex flex-wrap items-center gap-x-3 gap-y-1">
        <p className="min-w-0 flex-1">
          <span className="text-xs text-muted">Tour {step + 1} of {TOUR.length} · </span>
          <strong>{item.title}.</strong> {item.body(mod)}
        </p>
        <div className="flex items-center gap-1">
          {step > 0 && <button type="button" onClick={() => onStep(step - 1)} className="min-h-9 rounded-md border border-border px-2.5 text-xs hover:bg-surface-2">Back</button>}
          <button type="button" onClick={() => (last ? onClose() : onStep(step + 1))} className="min-h-9 rounded-md bg-accent px-2.5 text-xs font-medium text-accent-ink hover:brightness-110">{last ? "Done" : "Next"}</button>
          {!last && <button type="button" onClick={onClose} className="min-h-9 rounded-md px-2.5 text-xs text-muted hover:bg-surface-2 hover:text-text">Skip tour</button>}
        </div>
      </div>
    </section>
  );
}

function UndoToast({ onUndo, onDismiss }: { onUndo: () => void; onDismiss: () => void }) {
  // Like the achievement toasts, the timer pauses while the pointer or keyboard focus is on it.
  const [paused, setPaused] = useState(false);
  useEffect(() => {
    if (paused) return;
    const t = setTimeout(onDismiss, 8000);
    return () => clearTimeout(t);
  }, [paused, onDismiss]);
  return (
    <div role="status" className="pointer-events-none fixed inset-x-4 bottom-4 z-40 flex justify-center sm:left-4 sm:right-auto sm:justify-start">
      <div
        onMouseEnter={() => setPaused(true)} onMouseLeave={() => setPaused(false)} onFocus={() => setPaused(true)} onBlur={() => setPaused(false)}
        className="toast-in panel pointer-events-auto flex w-full max-w-sm items-center gap-3 p-3 shadow-lg"
      >
        <p className="min-w-0 flex-1 text-sm">Code reset to the starter.</p>
        <button type="button" onClick={onUndo} className="min-h-11 rounded-md px-3 text-sm font-medium text-accent underline underline-offset-2 hover:bg-surface-2 sm:min-h-9">Undo</button>
        <button type="button" onClick={onDismiss} aria-label="Dismiss" className="min-h-11 rounded-md px-2 text-sm text-muted hover:bg-surface-2 sm:min-h-9">✕</button>
      </div>
    </div>
  );
}

function summarize(r: RunResponse | null): string {
  if (!r) return "";
  const compileFailed = r.kind === "run" ? r.phase === "compile" && !r.ok : !r.compiled;
  if (compileFailed) return "Compile error";
  if (r.kind === "run") return r.ok ? "Program finished" : "Program failed";
  const passing = r.results.filter((t) => t.passed).length;
  return `${passing} of ${r.results.length} tests passing`;
}

function SuccessBanner({
  nextHref,
  nextLabel,
  signedIn,
  again = false,
  reward = null,
  paced = false,
  unaided = false,
  reviewed = false,
  variant,
}: {
  nextHref: string | null;
  nextLabel?: string;
  signedIn: boolean;
  again?: boolean;
  reward?: Reward | null;
  paced?: boolean;
  unaided?: boolean;
  reviewed?: boolean;
  variant: Variant;
}) {
  const noun = variant === "challenge" ? "challenge" : "lesson";
  return (
    <div role="status" className="rounded-lg bg-ok-soft p-4 text-sm">
      <p className="font-medium text-ok">
        {again ? `You've completed this ${noun}.` : "All tests pass — nicely done!"}
        {reward && <span className="ml-2 rounded-full bg-accent px-2 py-0.5 text-xs font-semibold text-accent-ink">+{reward.xp} XP</span>}
      </p>
      {unaided && <p className="mt-1">You earned the Unaided seal: no hints and no tutor.</p>}
      {variant === "review" && <p className="mt-1">{reviewed ? "Review recorded. Your next one is scheduled further out." : "Nicely done. This review wasn't due yet, so the schedule is unchanged."}</p>}
      {paced && <p className="mt-1">This lesson is saved as passed, but no XP was added because you passed lessons unusually fast or reached the daily XP limit. XP resumes automatically.</p>}
      {reward?.levelUp && (
        <p className="mt-1 font-medium">
          Level up! You&apos;re now level {reward.level}: {reward.title}.
        </p>
      )}
      {!signedIn && !again && (
        <p className="mt-1">
          <Link href="/signup" className="underline underline-offset-2">
            Create a free account
          </Link>{" "}
          to save this progress.
        </p>
      )}
      {nextHref && variant === "lesson" && (
        <Link href={nextHref} className="mt-3 inline-flex h-9 items-center rounded-lg bg-accent px-3 font-medium text-accent-ink">
          {nextLabel ?? "Next lesson →"}
        </Link>
      )}
    </div>
  );
}

function describeResult(r: RunResponse | null): string {
  if (!r) return "";
  if (r.kind === "run") {
    return [r.compileOutput && `Compiler:\n${r.compileOutput}`, r.error && `Error: ${r.error}`, r.phase === "run" && `stdout:\n${r.stdout}\nstderr:\n${r.stderr}\nexit code: ${r.exitCode}`]
      .filter(Boolean)
      .join("\n\n");
  }
  return [
    r.compileOutput && `Compiler:\n${r.compileOutput}`,
    r.error && `Error: ${r.error}`,
    r.results.map((t) => `${t.passed ? "PASS" : "FAIL"} ${t.name}${t.message ? `: ${t.message}` : ""}${t.expected !== undefined && t.actual !== undefined ? `\n  expected: ${t.expected}\n  actual: ${t.actual}` : ""}`).join("\n"),
    r.output && `Other output:\n${r.output}`,
  ]
    .filter(Boolean)
    .join("\n\n");
}
