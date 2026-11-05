"use client";

import { useEffect, useSyncExternalStore } from "react";

const subscribe = (notify: () => void) => {
  window.addEventListener("online", notify);
  window.addEventListener("offline", notify);
  return () => { window.removeEventListener("online", notify); window.removeEventListener("offline", notify); };
};

/** Retry button plus automatic reload when the browser reports the connection is back. */
export function OfflineActions() {
  const online = useSyncExternalStore(subscribe, () => navigator.onLine, () => false);
  useEffect(() => {
    const reload = () => location.reload();
    window.addEventListener("online", reload);
    return () => window.removeEventListener("online", reload);
  }, []);
  return (
    <div className="mt-6">
      <button type="button" onClick={() => location.reload()} className="inline-flex min-h-11 items-center rounded-lg bg-accent px-5 text-sm font-semibold text-accent-ink">Try again</button>
      <p role="status" className="mt-3 text-sm text-muted">{online ? "Connection looks available. Try again." : "Waiting for a connection. This page reloads on its own when you are back online."}</p>
    </div>
  );
}
