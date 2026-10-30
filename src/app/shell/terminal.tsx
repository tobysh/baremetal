"use client";

import "@xterm/xterm/css/xterm.css";
import { useCallback, useEffect, useRef, useState } from "react";
import { Terminal, type ITheme } from "@xterm/xterm";
import { FitAddon } from "@xterm/addon-fit";
import { Button } from "@/components/ui";

type Phase = "idle" | "connecting" | "live" | "ended";
type Notice = { tone: "info" | "err"; text: string };

// Messages for local failures; anything the broker says arrives with its own text.
const BY_STATUS: Record<number, string> = {
  401: "Your session expired. Sign in again.",
  403: "Verify your email address to use the shell.",
  409: "You already have a shell open, perhaps in another tab. Close it and try again.",
  429: "You have started too many shells recently. Try again in a few minutes.",
  503: "No shell is free right now. Try again in a minute.",
};

const css = (name: string) => getComputedStyle(document.documentElement).getPropertyValue(name).trim();

/** Terminal colours come from the site tokens, so the shell follows the theme. */
function readTheme(): ITheme {
  return {
    background: css("--code-bg"),
    foreground: css("--code-text"),
    cursor: css("--code-cursor"),
    cursorAccent: css("--code-bg"),
    selectionBackground: css("--code-selection"),
    black: css("--code-bg"),
    brightBlack: css("--code-muted"),
    red: css("--err"),
    green: css("--ok"),
    yellow: css("--code-keyword"),
    blue: css("--code-type"),
    magenta: css("--code-number"),
    cyan: css("--code-fn"),
    white: css("--code-text"),
    brightWhite: css("--code-text"),
  };
}

const sameOriginInit: RequestInit = { credentials: "same-origin", cache: "no-store" };

export default function ShellTerminal() {
  const host = useRef<HTMLDivElement>(null);
  const term = useRef<Terminal | null>(null);
  const fit = useRef<FitAddon | null>(null);
  const session = useRef<{ token: string; source: EventSource } | null>(null);
  const [phase, setPhase] = useState<Phase>("idle");
  const [notice, setNotice] = useState<Notice | null>(null);

  const post = useCallback((path: string, body: BodyInit, type: string) => {
    const s = session.current;
    if (!s) return Promise.resolve();
    return fetch(`/api/shell/${s.token}/${path}`, { ...sameOriginInit, method: "POST", headers: { "Content-Type": type }, body }).then((r) => {
      if (!r.ok && r.status !== 429) throw new Error("rejected");
    });
  }, []);

  const teardown = useCallback((closeRemote: boolean) => {
    const s = session.current;
    session.current = null;
    if (!s) return;
    s.source.close();
    if (closeRemote) void fetch(`/api/shell/${s.token}`, { ...sameOriginInit, method: "DELETE", keepalive: true }).catch(() => {});
  }, []);

  // Create the terminal once; keep it themed.
  useEffect(() => {
    const t = new Terminal({
      cursorBlink: true,
      fontFamily: getComputedStyle(host.current!).fontFamily || "ui-monospace, monospace",
      fontSize: 14,
      scrollback: 5000,
      minimumContrastRatio: 4.5,
      theme: readTheme(),
    });
    const f = new FitAddon();
    t.loadAddon(f);
    t.open(host.current!);
    f.fit();
    term.current = t;
    fit.current = f;
    const retheme = () => { t.options.theme = readTheme(); };
    const media = matchMedia("(prefers-color-scheme: dark)");
    media.addEventListener("change", retheme);
    const observer = new MutationObserver(retheme);
    observer.observe(document.documentElement, { attributes: true, attributeFilter: ["data-theme", "class"] });
    return () => {
      media.removeEventListener("change", retheme);
      observer.disconnect();
      teardown(true);
      t.dispose();
      term.current = null;
    };
  }, [teardown]);

  // Keystrokes: coalesce within a tick, send in order, in chunks under the server's limit.
  useEffect(() => {
    const t = term.current!;
    let pending = "";
    let chain: Promise<void> = Promise.resolve();
    let scheduled = false;
    const encoder = new TextEncoder();
    const flush = () => {
      scheduled = false;
      const bytes = encoder.encode(pending);
      pending = "";
      for (let i = 0; i < bytes.length; i += 3000) {
        const part = bytes.slice(i, i + 3000);
        chain = chain.then(() => post("input", part, "application/octet-stream")).catch(() => {});
      }
    };
    const queue = (data: string) => {
      if (!session.current) return;
      pending += data;
      if (!scheduled) { scheduled = true; setTimeout(flush, 5); }
    };
    const a = t.onData(queue);
    const b = t.onBinary((data) => queue(data));
    return () => { a.dispose(); b.dispose(); };
  }, [post]);

  // Resize: refit on container changes, tell the guest (debounced).
  useEffect(() => {
    let timer: ReturnType<typeof setTimeout> | undefined;
    const observer = new ResizeObserver(() => {
      fit.current?.fit();
      clearTimeout(timer);
      timer = setTimeout(() => {
        const t = term.current;
        if (t && session.current) void post("resize", JSON.stringify({ cols: t.cols, rows: t.rows }), "application/json").catch(() => {});
      }, 150);
    });
    observer.observe(host.current!);
    return () => { clearTimeout(timer); observer.disconnect(); };
  }, [post]);

  const end = useCallback((n: Notice) => {
    teardown(true);
    setNotice(n);
    setPhase("ended");
  }, [teardown]);

  const start = useCallback(async () => {
    const t = term.current!;
    t.reset();
    fit.current?.fit();
    setNotice(null);
    setPhase("connecting");
    try {
      const res = await fetch("/api/shell", {
        ...sameOriginInit,
        method: "POST",
        headers: { "Content-Type": "application/json" },
        body: JSON.stringify({ cols: t.cols, rows: t.rows }),
      });
      if (!res.ok) {
        const body = (await res.json().catch(() => ({}))) as { error?: string };
        setNotice({ tone: "err", text: body.error || BY_STATUS[res.status] || "The shell could not start." });
        return setPhase("ended");
      }
      const { token } = (await res.json()) as { token: string };
      const source = new EventSource(`/api/shell/${token}`);
      session.current = { token, source };
      let finished = false;
      source.addEventListener("ready", () => { setPhase("live"); t.focus(); });
      source.onmessage = (e) => {
        const bin = atob(e.data);
        const bytes = Uint8Array.from(bin, (c) => c.charCodeAt(0));
        t.write(bytes);
      };
      source.addEventListener("exit", () => { finished = true; end({ tone: "info", text: "The shell exited." }); });
      source.addEventListener("shell-error", (e) => {
        finished = true;
        let text = "The shell ended unexpectedly.";
        try { text = String((JSON.parse((e as MessageEvent).data) as { message?: string }).message || text); } catch {}
        end({ tone: "err", text });
      });
      // EventSource would silently reconnect; the session token is single-use, so treat any drop as the end.
      source.onerror = () => { if (!finished) end({ tone: "err", text: "Disconnected from the shell." }); };
    } catch {
      setNotice({ tone: "err", text: "Could not reach the server. Check your connection and try again." });
      setPhase("ended");
    }
  }, [end]);

  const live = phase === "live" || phase === "connecting";
  return (
    <section aria-label="Terminal" className="space-y-3">
      <div className="flex flex-wrap items-center gap-3">
        {live ? (
          <Button variant="secondary" size="sm" onClick={() => end({ tone: "info", text: "You closed the shell." })}>Close shell</Button>
        ) : (
          <Button size="sm" onClick={() => void start()}>{phase === "ended" ? "Start a new shell" : "Start shell"}</Button>
        )}
        <p role="status" aria-live="polite" className={`text-sm ${notice?.tone === "err" ? "text-err" : "text-muted"}`}>
          {phase === "connecting" && "Starting your machine, this takes a few seconds."}
          {phase === "live" && "Connected. Tab is sent to the shell; use Shift+Tab to leave the terminal."}
          {phase === "idle" && "Not started."}
          {notice?.text}
        </p>
      </div>
      <div className="overflow-hidden rounded-2xl border border-border bg-[var(--code-bg)] p-2">
        <div ref={host} className="h-[28rem] w-full font-mono" />
      </div>
    </section>
  );
}
