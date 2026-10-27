"use client";

import { usePathname } from "next/navigation";
import { useEffect, useRef } from "react";
import { getConsent, useConsent } from "./consent";

type EventName = "pageview" | "signup_cta" | "run" | "test_passed" | "tutor_open" | "hint_open";

/** Sends an analytics event — only if the visitor has opted in. */
export function track(type: EventName) {
  if (!getConsent()?.analytics) return;
  const body = JSON.stringify({ type, path: location.pathname, ref: document.referrer || undefined });
  if (!navigator.sendBeacon?.("/api/analytics", new Blob([body], { type: "application/json" }))) {
    fetch("/api/analytics", { method: "POST", body, keepalive: true, headers: { "Content-Type": "application/json" } }).catch(() => {});
  }
}

export function Analytics() {
  const pathname = usePathname();
  const consent = useConsent();
  const last = useRef<string | null>(null);

  useEffect(() => {
    if (!consent?.analytics || last.current === pathname) return;
    last.current = pathname;
    track("pageview");
  }, [pathname, consent?.analytics]);

  return null;
}
