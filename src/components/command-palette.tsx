"use client";

import dynamic from "next/dynamic";
import { useEffect, useState, useSyncExternalStore } from "react";

// The dialog and its search code are only fetched the first time the palette opens.
const PaletteDialog = dynamic(() => import("./command-palette-dialog"), { ssr: false });

export function CommandPalette({ signedIn, username }: { signedIn: boolean; username: string | null }) {
  const [opened, setOpened] = useState(false); // has ever been opened: keeps the dialog mounted afterwards
  const [open, setOpen] = useState(false);
  const mac = useSyncExternalStore(() => () => {}, () => /mac|iphone|ipad/i.test(navigator.platform || navigator.userAgent), () => false);

  useEffect(() => {
    const onKey = (event: KeyboardEvent) => {
      if ((event.metaKey || event.ctrlKey) && !event.altKey && !event.shiftKey && event.key.toLowerCase() === "k") {
        event.preventDefault();
        setOpened(true);
        setOpen((v) => !v);
      }
    };
    window.addEventListener("keydown", onKey);
    return () => window.removeEventListener("keydown", onKey);
  }, []);

  return (
    <>
      <button
        type="button"
        onClick={() => { setOpened(true); setOpen(true); }}
        aria-label="Search lessons and pages"
        aria-haspopup="dialog"
        className="inline-flex min-h-11 items-center gap-2 rounded-lg border border-line-strong px-3 text-sm text-muted hover:bg-surface-2 hover:text-text"
      >
        <svg aria-hidden="true" viewBox="0 0 16 16" fill="none" stroke="currentColor" strokeWidth="1.5" strokeLinecap="round" className="h-4 w-4"><circle cx="7" cy="7" r="4.5" /><path d="m10.5 10.5 3 3" /></svg>
        <span className="hidden xl:inline">Search</span>
        <kbd aria-hidden="true" className="hidden rounded border border-border px-1.5 font-mono text-xs xl:inline">{mac ? "⌘K" : "Ctrl K"}</kbd>
      </button>
      {opened && <PaletteDialog open={open} onClose={() => setOpen(false)} signedIn={signedIn} username={username} />}
    </>
  );
}
