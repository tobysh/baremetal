"use client";

import Link from "next/link";
import { useRouter } from "next/navigation";
import { type KeyboardEvent, Suspense, lazy, useCallback, useLayoutEffect, useMemo, useRef, useState, useSyncExternalStore } from "react";
import { LANGUAGE_FILE, LANGUAGE_LABEL, type PublicExercise } from "@/lib/curriculum/types";
import { track } from "../analytics";
import { MiniMarkdown } from "./mini-markdown";
import { explainFailure, type Explanation } from "@/lib/diagnostics";
import { errorLines } from "@/lib/error-lines";
import { TutorChat, type ChatMessage, type TutorControl } from "./tutor-chat";

// The editor is the heaviest dependency on the site: load it only in the
// browser, only on lesson pages, and show the code as plain text until then
// (so the server-rendered HTML contains the starter code too).
const NO_LINES: number[] = [];
const CodeEditor = lazy(() => import("./code-editor"));
const noop = () => () => {};
const useIsClient = () => useSyncExternalStore(noop, () => true, () => false);

function readDraft(key: string): string | null {
  try {
    return localStorage.getItem(key);
  } catch {
    return null;
  }
}

// Matches the editor's colours so there's no flash while it loads.
function CodeFallback({ code }: { code: string }) {
  return <pre className="h-full overflow-auto bg-[var(--code-bg)] p-4 pl-12 font-mono text-[15px] leading-[1.65] text-[var(--code-text)]">{code}</pre>;
}

const isMac = () => /Mac|iPhone|iPad/.test(navigator.platform || navigator.userAgent);
const useModKey = () => useSyncExternalStore(noop, () => (isMac() ? "⌘" : "Ctrl"), () => "Ctrl");

function Kbd({ children }: { children: string }) {
  return <kbd className="rounded border border-current/30 px-1 font-sans text-[10px] leading-4 opacity-80">{children}</kbd>;
}

type TestCase = { name: string; passed: boolean; message?: string };
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
    };

type Reward = { xp: number; totalXp: number; level: number; title: string; levelUp: boolean };

type Tab = "output" | "hints" | "tutor";

export function Workspace({
  exercise,
  initialCode,
  signedIn,
  alreadyPassed,
  tutorHistory,
  nextHref,
  loginHref,
}: {
  exercise: PublicExercise;
  initialCode: string | null;
  signedIn: boolean;
  alreadyPassed: boolean;
  tutorHistory: ChatMessage[];
  nextHref: string | null;
  loginHref: string;
}) {
  const storageKey = `bm:code:${exercise.id}`;
  // Signed-out visitors keep their code in this browser only.
  const localDraft = useSyncExternalStore(noop, () => (signedIn ? null : readDraft(storageKey)), () => null);
  const [edited, setCode] = useState<string | null>(null);
  const code = edited ?? localDraft ?? initialCode ?? exercise.starter;
  const isClient = useIsClient();
  const mod = useModKey();
  const [stdin, setStdin] = useState("");
  const [busy, setBusy] = useState<null | "run" | "test">(null);
  const router = useRouter();
  const [result, setResult] = useState<RunResponse | null>(null);
  const [requestError, setRequestError] = useState<string | null>(null);
  const [passed, setPassed] = useState(alreadyPassed);
  const [tab, setTab] = useState<Tab>("output");
  const [hintsShown, setHintsShown] = useState(0);
  const [saveState, setSaveState] = useState<"idle" | "saving" | "saved" | "error">("idle");
  const [confirmReset, setConfirmReset] = useState(false);
  const tutorRef = useRef<TutorControl>(null);
  const saveTimer = useRef<ReturnType<typeof setTimeout> | null>(null);
  const dirty = useRef(false);

  const onChange = useCallback(
    (value: string) => {
      setCode(value);
      dirty.current = true;
      if (saveTimer.current) clearTimeout(saveTimer.current);
      saveTimer.current = setTimeout(async () => {
        if (!signedIn) {
          try {
            localStorage.setItem(storageKey, value);
          } catch {}
          return;
        }
        setSaveState("saving");
        try {
          const res = await fetch("/api/progress", {
            method: "PUT",
            headers: { "Content-Type": "application/json" },
            body: JSON.stringify({ exerciseId: exercise.id, code: value }),
          });
          setSaveState(res.ok ? "saved" : "error");
        } catch {
          setSaveState("error");
        }
      }, 1200);
    },
    [exercise.id, signedIn, storageKey],
  );

  // Latest values for callbacks handed to the editor's keymap.
  const codeRef = useRef(code);
  const stdinRef = useRef(stdin);
  useLayoutEffect(() => {
    codeRef.current = code;
    stdinRef.current = stdin;
  }, [code, stdin]);

  const execute = useCallback(
    async (mode: "run" | "test") => {
      setBusy(mode);
      setRequestError(null);
      setTab("output");
      track("run");
      try {
        const res = await fetch("/api/run", {
          method: "POST",
          headers: { "Content-Type": "application/json" },
          body: JSON.stringify({ exerciseId: exercise.id, code: codeRef.current, stdin: stdinRef.current, mode }),
        });
        const body = await res.json();
        if (!res.ok) throw new Error(body.error ?? "Something went wrong running your code.");
        setResult(body);
        if (body.kind === "test" && body.allPassed) {
          setPassed(true);
          track("test_passed");
          if (body.reward) router.refresh(); // header XP chip reads the user at request time
        }
      } catch (e) {
        setRequestError((e as Error).message);
      } finally {
        setBusy(null);
      }
    },
    [exercise.id, router],
  );
  const run = useCallback(() => execute("run"), [execute]);
  const test = useCallback(() => execute("test"), [execute]);

  const errorLinesOf = useMemo(() => {
    if (!result || busy) return NO_LINES;
    const compileFailed = result.kind === "run" ? result.phase === "compile" && !result.ok : !result.compiled;
    return errorLines(exercise.language, result.compileOutput, result.kind === "run" ? result.stderr : result.output, compileFailed);
  }, [result, busy, exercise.language]);
  const lastOutput = describeResult(result);
  const getContext = useCallback(() => ({ code: codeRef.current, lastOutput }), [lastOutput]);

  const askTutor = (text: string) => {
    setTab("tutor");
    if (signedIn) tutorRef.current?.send(text);
  };

  const tabs: { id: Tab; label: string }[] = [
    { id: "output", label: "Output" },
    { id: "hints", label: `Hints (${hintsShown}/${exercise.hints.length})` },
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
    <div onKeyDown={onKeyDown} className="flex flex-col overflow-clip panel lg:h-[calc(100dvh-10rem)] lg:min-h-[32rem]">
      <div className="sticky top-16 z-10 flex shrink-0 flex-wrap items-center gap-2 border-b border-border bg-surface px-3 py-3 lg:static">
        <span className="font-mono text-xs text-muted">{LANGUAGE_FILE[exercise.language]}</span>
        <span className="text-xs text-muted" aria-live="polite">
          {signedIn ? { idle: "", saving: "Saving…", saved: "Saved", error: "Couldn't save" }[saveState] : "Saved in this browser only"}
        </span>
        <div className="ml-auto flex flex-wrap items-center justify-end gap-2">
          {confirmReset ? (
            <>
              <span className="text-xs text-muted">Discard your code?</span>
              <button
                type="button"
                className="min-h-11 rounded-md px-2 text-xs text-err hover:bg-err-soft sm:min-h-9"
                onClick={() => {
                  onChange(exercise.starter);
                  setConfirmReset(false);
                }}
              >
                Reset
              </button>
              <button type="button" className="min-h-11 rounded-md px-2 text-xs text-muted hover:bg-surface-2 sm:min-h-9" onClick={() => setConfirmReset(false)}>
                Cancel
              </button>
            </>
          ) : (
            <button type="button" className="min-h-11 rounded-md px-2 text-xs text-muted hover:bg-surface-2 hover:text-text sm:min-h-9" onClick={() => setConfirmReset(true)}>
              Reset
            </button>
          )}
          <button
            type="button"
            onClick={run}
            disabled={!!busy}
            title={`Compile and run your own main() (${mod}+Shift+Enter)`}
            aria-keyshortcuts={mod === "⌘" ? "Meta+Shift+Enter" : "Control+Shift+Enter"}
            className="inline-flex h-11 items-center gap-2 rounded-lg border border-border px-3 text-sm font-medium hover:bg-surface-2 disabled:opacity-60 sm:h-9"
          >
            {busy === "run" ? "Running…" : "Run"}
            <span className="hidden items-center gap-0.5 sm:inline-flex">
              <Kbd>{mod}</Kbd>
              <Kbd>⇧</Kbd>
              <Kbd>↵</Kbd>
            </span>
          </button>
          <button
            type="button"
            onClick={test}
            disabled={!!busy}
            title={`Check your solution against the tests (${mod}+Enter)`}
            aria-keyshortcuts={mod === "⌘" ? "Meta+Enter" : "Control+Enter"}
            className="inline-flex h-11 items-center gap-2 rounded-lg bg-accent px-3 text-sm font-medium text-accent-ink hover:brightness-110 disabled:opacity-60 sm:h-9"
          >
            {busy === "test" ? "Testing…" : "Test"}
            <span className="hidden items-center gap-0.5 sm:inline-flex">
              <Kbd>{mod}</Kbd>
              <Kbd>↵</Kbd>
            </span>
          </button>
        </div>
      </div>

      <div className="h-[22rem] min-h-0 border-b border-border lg:h-auto lg:flex-[1.3]">
        {isClient ? (
          <Suspense fallback={<CodeFallback code={code} />}>
            <CodeEditor value={code} onChange={onChange} language={exercise.language} onRun={run} onTest={test} label={`${LANGUAGE_LABEL[exercise.language]} code editor`} errorLines={errorLinesOf} />
          </Suspense>
        ) : (
          <CodeFallback code={code} />
        )}
      </div>

      <p className="hidden border-b border-border px-3 py-1.5 text-[11px] text-muted sm:block">
        <Kbd>{mod}</Kbd>+<Kbd>↵</Kbd> Test · <Kbd>{mod}</Kbd>+<Kbd>⇧</Kbd>+<Kbd>↵</Kbd> Run · <Kbd>Tab</Kbd> indent · <Kbd>{mod}</Kbd>+<Kbd>/</Kbd> comment ·{" "}
        <Kbd>Esc</Kbd> then <Kbd>Tab</Kbd> leave editor
      </p>

      {exercise.mode === "stdout" && (
        <div className="border-b border-border px-3 py-2">
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
            className={`-mb-px min-h-11 border-b-2 px-3 py-2 text-sm ${tab === t.id ? "border-accent text-text" : "border-transparent text-muted hover:text-text"}`}
          >
            {t.label}
          </button>
        ))}
      </div>

      <div className="h-[20rem] min-h-0 lg:h-auto lg:flex-1">
        <div role="tabpanel" id="panel-output" aria-labelledby="tab-output" tabIndex={0} hidden={tab !== "output"} className="h-full overflow-y-auto p-4">
          {result && !busy && <CopyOutput text={lastOutput} />}
          <OutputPanel
            result={result}
            busy={busy}
            requestError={requestError}
            passed={passed}
            nextHref={nextHref}
            signedIn={signedIn}
            onAsk={askTutor}
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
              })
            }
          />
        </div>
        <div role="tabpanel" id="panel-hints" aria-labelledby="tab-hints" tabIndex={0} hidden={tab !== "hints"} className="h-full overflow-y-auto p-4">
          <ol className="space-y-3">
            {exercise.hints.slice(0, hintsShown).map((h, i) => (
              <li key={i} className="rounded-lg border border-border bg-bg p-3 text-sm">
                <p className="mb-1 text-xs font-medium text-muted">Hint {i + 1}</p>
                <MiniMarkdown text={h} />
              </li>
            ))}
          </ol>
          {hintsShown < exercise.hints.length ? (
            <button
              type="button"
              className="mt-3 h-9 rounded-lg border border-border px-3 text-sm hover:bg-surface-2"
              onClick={() => {
                setHintsShown((n) => n + 1);
                track("hint_open");
              }}
            >
              {hintsShown === 0 ? "Show a hint" : "Show the next hint"}
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
              hintsShown < exercise.hints.length
                ? () => {
                    // Authored hints cost nothing — offer them before a model call.
                    setHintsShown((n) => n + 1);
                    setTab("hints");
                    track("hint_open");
                  }
                : null
            }
          />
        </div>
      </div>
    </div>
  );
}

function CopyOutput({ text }: { text: string }) {
  const [status, setStatus] = useState("Copy output");
  return (
    <div className="mb-3 flex justify-end">
      <button type="button" className="min-h-9 rounded-lg border border-border px-3 text-xs text-muted hover:bg-surface-2 hover:text-text" onClick={async () => {
        try { await navigator.clipboard.writeText(text); setStatus("Copied"); }
        catch { setStatus("Copy unavailable"); }
      }}><span aria-live="polite">{status}</span></button>
    </div>
  );
}

function OutputPanel({
  result,
  busy,
  requestError,
  passed,
  nextHref,
  signedIn,
  onAsk,
  explain,
}: {
  result: RunResponse | null;
  busy: null | "run" | "test";
  requestError: string | null;
  passed: boolean;
  nextHref: string | null;
  signedIn: boolean;
  onAsk: (text: string) => void;
  explain: (r: RunResponse) => Explanation[];
}) {
  if (busy) {
    return (
      <div className="space-y-2" aria-live="polite">
        <p className="text-sm text-muted">{busy === "test" ? "Compiling and running the tests…" : "Compiling and running…"}</p>
        <div className="skeleton h-4 w-2/3" />
        <div className="skeleton h-4 w-1/2" />
      </div>
    );
  }
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
        {passed && <SuccessBanner nextHref={nextHref} signedIn={signedIn} again />}
      </div>
    );
  }

  const compileFailed = result.kind === "run" ? result.phase === "compile" && !result.ok : !result.compiled;
  const failed = compileFailed || !!result.error || (result.kind === "test" && !result.allPassed);
  const explanations = failed ? explain(result) : [];
  return (
    <div className="space-y-4" aria-live="polite">
      {result.kind === "test" && result.allPassed && <SuccessBanner nextHref={nextHref} signedIn={signedIn} reward={result.reward} />}

      {explanations.length > 0 && (
        <section aria-labelledby="what-went-wrong" className="space-y-2">
          <h3 id="what-went-wrong" className="text-xs font-medium text-muted">
            What probably went wrong
          </h3>
          {explanations.map((x) => (
            <div key={x.id} className="rounded-lg border border-border bg-bg p-3">
              <p className="text-sm font-medium">{x.title}</p>
              <MiniMarkdown text={x.body} />
              <button
                type="button"
                onClick={() => onAsk(`The checker said: "${x.title}". Can you look at my code and explain what I did wrong here, without giving me the full answer?`)}
                className="mt-2 inline-flex h-8 items-center rounded-md border border-border px-2.5 text-xs font-medium hover:bg-surface-2"
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
            {result.results.map((r, i) => (
              <li key={i} className="text-sm">
                <span className={r.passed ? "text-ok" : "text-err"}>
                  <span aria-hidden="true">{r.passed ? "✓" : "✗"}</span>
                  <span className="sr-only">{r.passed ? "Passed:" : "Failed:"}</span> {r.name}
                </span>
                {r.message && <pre className="mt-1 ml-5 overflow-auto rounded-md bg-bg p-2 font-mono text-xs whitespace-pre-wrap text-muted">{r.message}</pre>}
              </li>
            ))}
          </ul>
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

function SuccessBanner({ nextHref, signedIn, again = false, reward = null }: { nextHref: string | null; signedIn: boolean; again?: boolean; reward?: Reward | null }) {
  return (
    <div role="status" className="rounded-lg bg-ok-soft p-4 text-sm">
      <p className="font-medium text-ok">
        {again ? "You've completed this lesson." : "All tests pass — nicely done!"}
        {reward && <span className="ml-2 rounded-full bg-accent px-2 py-0.5 text-xs font-semibold text-accent-ink">+{reward.xp} XP</span>}
      </p>
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
      {nextHref && (
        <Link href={nextHref} className="mt-3 inline-flex h-9 items-center rounded-lg bg-accent px-3 font-medium text-accent-ink">
          Next lesson →
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
    r.results.map((t) => `${t.passed ? "PASS" : "FAIL"} ${t.name}${t.message ? `: ${t.message}` : ""}`).join("\n"),
    r.output && `Other output:\n${r.output}`,
  ]
    .filter(Boolean)
    .join("\n\n");
}
