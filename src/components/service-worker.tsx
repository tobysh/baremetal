"use client";

import { useEffect } from "react";

/** Registers the scoped service worker (public/sw.js) in production only. See the notes at the top of that file for what it caches. */
export function ServiceWorker() {
  useEffect(() => {
    if (process.env.NODE_ENV !== "production" || !("serviceWorker" in navigator)) return;
    navigator.serviceWorker.register("/sw.js", { scope: "/" }).catch(() => {});
  }, []);
  return null;
}
