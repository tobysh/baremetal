"use client";

import Link from "next/link";
import { useState, useSyncExternalStore } from "react";

const subscribe = () => () => {};
export function UsernameNotice({ userId }: { userId: string }) {
  const key = `bm:username-notice:${userId}`;
  const stored = useSyncExternalStore(subscribe, () => {
    try { return localStorage.getItem(key) === "dismissed"; } catch { return false; }
  }, () => true);
  const [dismissed, setDismissed] = useState(false);
  if (stored || dismissed) return null;
  return (
    <div className="mt-8 flex items-center justify-between gap-4 rounded-lg bg-surface-2 px-4 py-3 text-sm text-muted">
      <p>Make your progress yours. <Link href="/settings" className="text-text underline underline-offset-4">Choose a username</Link> for your public profile.</p>
      <button type="button" aria-label="Dismiss username reminder" className="flex h-11 w-11 shrink-0 items-center justify-center rounded-lg hover:bg-surface hover:text-text" onClick={() => {
        setDismissed(true);
        try { localStorage.setItem(key, "dismissed"); } catch {}
      }}>×</button>
    </div>
  );
}
