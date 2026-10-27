"use client";

import Link from "next/link";
import { type Ref, useEffect, useImperativeHandle, useRef, useState } from "react";
import { track } from "../analytics";
import { MiniMarkdown } from "./mini-markdown";

export type ChatMessage = { role: "user" | "assistant"; content: string };
/** Lets the workspace send a question on the learner's behalf (e.g. "Elaborate"). */
export type TutorControl = { send: (text: string) => void };

const ERROR_MARKER = "__TUTOR_ERROR__";

export function TutorChat({
  exerciseId,
  signedIn,
  initialMessages,
  getContext,
  loginHref,
  onShowHint,
  controlRef,
}: {
  exerciseId: string;
  signedIn: boolean;
  initialMessages: ChatMessage[];
  getContext: () => { code: string; lastOutput: string };
  loginHref: string;
  onShowHint: (() => void) | null;
  controlRef: Ref<TutorControl>;
}) {
  const [messages, setMessages] = useState<ChatMessage[]>(initialMessages);
  const [input, setInput] = useState("");
  const [busy, setBusy] = useState(false);
  const [error, setError] = useState<string | null>(null);
  const [quota, setQuota] = useState<{ remaining: number; limit: number } | null>(null);
  const listRef = useRef<HTMLDivElement>(null);
  const inputRef = useRef<HTMLTextAreaElement>(null);

  useEffect(() => {
    listRef.current?.scrollTo({ top: listRef.current.scrollHeight });
  }, [messages]);

  useImperativeHandle(controlRef, () => ({ send: (text: string) => void send(text) }));

  async function send(text: string) {
    const message = text.trim();
    if (!message || busy) return;
    setBusy(true);
    setError(null);
    setInput("");
    setMessages((m) => [...m, { role: "user", content: message }, { role: "assistant", content: "" }]);
    track("tutor_open");
    try {
      const res = await fetch("/api/tutor", {
        method: "POST",
        headers: { "Content-Type": "application/json" },
        body: JSON.stringify({ exerciseId, message, ...getContext() }),
      });
      const remaining = res.headers.get("X-Tutor-Remaining");
      const limit = res.headers.get("X-Tutor-Limit");
      if (remaining !== null && limit !== null) setQuota({ remaining: Number(remaining), limit: Number(limit) });
      if (!res.ok || !res.body) {
        const body = await res.json().catch(() => ({}));
        throw new Error(body.error ?? "The tutor couldn't answer just now.");
      }
      const reader = res.body.getReader();
      const decoder = new TextDecoder();
      let reply = "";
      for (;;) {
        const { done, value } = await reader.read();
        if (done) break;
        reply += decoder.decode(value, { stream: true });
        const visible = reply.split(ERROR_MARKER)[0];
        setMessages((m) => [...m.slice(0, -1), { role: "assistant", content: visible }]);
      }
      if (reply.includes(ERROR_MARKER)) throw new Error(reply.split(ERROR_MARKER)[1]);
    } catch (e) {
      setError((e as Error).message);
      setMessages((m) => (m.at(-1)?.role === "assistant" && !m.at(-1)?.content ? m.slice(0, -2) : m));
      setInput(message);
    } finally {
      setBusy(false);
    }
  }

  async function clear() {
    await fetch(`/api/tutor?exerciseId=${encodeURIComponent(exerciseId)}`, { method: "DELETE" });
    setMessages([]);
  }

  if (!signedIn) {
    return (
      <div className="flex h-full flex-col items-start justify-center gap-3 p-6">
        <h3 className="font-semibold">Stuck? Ask the tutor.</h3>
        <p className="text-sm text-muted">
          The AI tutor reads your code and errors and helps you work out the answer yourself. It&apos;s free with an account.
        </p>
        <div className="flex gap-2">
          <Link href="/signup" className="inline-flex h-9 items-center rounded-lg bg-accent px-3 text-sm font-medium text-accent-ink" onClick={() => track("signup_cta")}>
            Create free account
          </Link>
          <Link href={loginHref} className="inline-flex h-9 items-center rounded-lg border border-border px-3 text-sm">
            Sign in
          </Link>
        </div>
      </div>
    );
  }

  return (
    <div className="flex h-full min-h-0 flex-col">
      <div ref={listRef} className="min-h-0 flex-1 space-y-4 overflow-y-auto p-4" aria-live="polite" aria-busy={busy}>
        {messages.length === 0 && (
          <div className="text-sm text-muted">
            <p>Ask anything about this lesson — what an error means, why your code behaves the way it does, or what to try next.</p>
            <div className="mt-3 flex flex-wrap gap-2">
              {onShowHint && (
                <button type="button" onClick={onShowHint} className="rounded-full border border-accent/50 bg-accent-soft px-3 py-1 text-xs text-text hover:brightness-110">
                  Show me the next hint
                </button>
              )}
              {["I don't know where to start", "Explain my error", "Is my approach on the right track?"].map((q) => (
                <button key={q} type="button" onClick={() => send(q)} className="rounded-full border border-border px-3 py-1 text-xs text-text hover:bg-surface-2">
                  {q}
                </button>
              ))}
            </div>
          </div>
        )}
        {messages.map((m, i) =>
          m.role === "user" ? (
            <div key={i} className="ml-8 rounded-xl bg-surface-2 px-3 py-2 text-sm whitespace-pre-wrap">
              {m.content}
            </div>
          ) : (
            <div key={i} className="mr-4">
              {m.content ? <MiniMarkdown text={m.content} /> : <span className="text-sm text-muted">Thinking…</span>}
            </div>
          ),
        )}
      </div>
      {error && (
        <p role="alert" className="mx-4 mb-2 rounded-lg bg-err-soft px-3 py-2 text-sm text-err">
          {error}
        </p>
      )}
      <form
        className="flex items-end gap-2 border-t border-border p-3"
        onSubmit={(e) => {
          e.preventDefault();
          send(input);
        }}
      >
        <label htmlFor="tutor-input" className="sr-only">
          Message the tutor
        </label>
        <textarea
          id="tutor-input"
          ref={inputRef}
          rows={2}
          maxLength={4000}
          value={input}
          onChange={(e) => setInput(e.target.value)}
          onKeyDown={(e) => {
            if (e.key === "Enter" && !e.shiftKey) {
              e.preventDefault();
              send(input);
            }
          }}
          placeholder="Ask the tutor… (Enter to send)"
          className="min-h-[2.5rem] flex-1 resize-none rounded-lg border border-border bg-bg px-3 py-2 text-sm placeholder:text-muted"
        />
        <div className="flex flex-col gap-1">
          <button type="submit" disabled={busy || !input.trim()} className="h-9 rounded-lg bg-accent px-3 text-sm font-medium text-accent-ink disabled:opacity-50">
            {busy ? "…" : "Send"}
          </button>
          {messages.length > 0 && (
            <button type="button" onClick={clear} disabled={busy} className="text-xs text-muted hover:text-text">
              Clear
            </button>
          )}
        </div>
      </form>
      {quota && (
        <p className="px-3 pb-2 text-[11px] text-muted" aria-live="polite">
          {quota.remaining} of {quota.limit} tutor questions left this hour. Repeated common questions and hints don&apos;t count.
        </p>
      )}
    </div>
  );
}
