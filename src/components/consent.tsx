"use client";

import Link from "next/link";
import { useEffect, useState, useSyncExternalStore } from "react";
import { CONSENT_COOKIE, CONSENT_MAX_AGE, parseConsent, serializeConsent, type Consent } from "@/lib/consent";
import { Button } from "./ui";

const OPEN_EVENT = "bm:open-consent";
const CHANGE_EVENT = "bm:consent-changed";

function readCookie(): string | null {
  const match = document.cookie.split("; ").find((c) => c.startsWith(`${CONSENT_COOKIE}=`));
  return match ? match.slice(CONSENT_COOKIE.length + 1) : null;
}

export function getConsent(): Consent | null {
  if (typeof document === "undefined") return null;
  return parseConsent(readCookie());
}

function saveConsent(analytics: boolean) {
  const secure = location.protocol === "https:" ? "; Secure" : "";
  document.cookie = `${CONSENT_COOKIE}=${serializeConsent(analytics)}; Max-Age=${CONSENT_MAX_AGE}; Path=/; SameSite=Lax${secure}`;
  window.dispatchEvent(new Event(CHANGE_EVENT));
}

function subscribe(cb: () => void) {
  window.addEventListener(CHANGE_EVENT, cb);
  return () => window.removeEventListener(CHANGE_EVENT, cb);
}

/** Current consent, re-rendering when it changes. `undefined` during SSR. */
export function useConsent(): Consent | null | undefined {
  const raw = useSyncExternalStore(subscribe, () => readCookie(), () => undefined);
  return raw === undefined ? undefined : parseConsent(raw);
}

export function CookieSettingsButton() {
  return (
    <button type="button" className="hover:text-text" onClick={() => window.dispatchEvent(new Event(OPEN_EVENT))}>
      Cookie settings
    </button>
  );
}

export function CookieBanner() {
  const consent = useConsent();
  const [forcedOpen, setForcedOpen] = useState(false);
  const [analytics, setAnalytics] = useState(false);

  useEffect(() => {
    const open = () => {
      setAnalytics(!!getConsent()?.analytics);
      setForcedOpen(true);
    };
    window.addEventListener(OPEN_EVENT, open);
    return () => window.removeEventListener(OPEN_EVENT, open);
  }, []);

  // Not rendered on the server or until we know there's no stored choice.
  if (consent === undefined || (consent !== null && !forcedOpen)) return null;

  const choose = (value: boolean) => {
    saveConsent(value);
    setForcedOpen(false);
  };

  return (
    <section
      role="region"
      aria-label="Cookie preferences"
      className="fixed inset-x-0 bottom-0 z-50 border-t border-border bg-surface text-text"
    >
      <div className="mx-auto flex w-full max-w-6xl flex-col gap-2 px-4 py-2.5 sm:flex-row sm:items-center sm:gap-4 sm:px-6 lg:px-8">
        <div className="min-w-0 flex-1 text-xs leading-snug text-muted sm:text-sm">
          <h2 className="inline font-semibold text-text">Cookies and analytics.</h2>{" "}
          Essential cookies keep you signed in. With your permission we also count page visits, privately and without ads.{" "}
          <Link href="/cookies" className="text-accent underline underline-offset-2">
            Cookie Policy
          </Link>
          {forcedOpen && (
            <span className="mt-1.5 flex flex-wrap gap-x-4 gap-y-1 text-text">
              <label className="flex items-center gap-2">
                <input type="checkbox" checked disabled className="h-4 w-4 accent-[var(--accent)]" />
                <span>Essential <span className="text-muted">(always on)</span></span>
              </label>
              <label className="flex items-center gap-2">
                <input type="checkbox" checked={analytics} onChange={(e) => setAnalytics(e.target.checked)} className="h-4 w-4 accent-[var(--accent)]" />
                <span>Analytics</span>
              </label>
            </span>
          )}
        </div>
        {/* Accept and reject are equally prominent, as GDPR guidance requires. */}
        <div className="flex flex-wrap gap-2 sm:shrink-0">
          {forcedOpen ? (
            <Button size="sm" variant="secondary" onClick={() => choose(analytics)}>
              Save choices
            </Button>
          ) : (
            <>
              <Button size="sm" variant="secondary" onClick={() => choose(false)}>
                Reject analytics
              </Button>
              <Button size="sm" variant="secondary" onClick={() => choose(true)}>
                Accept analytics
              </Button>
              <Button
                size="sm"
                variant="ghost"
                onClick={() => {
                  setAnalytics(false);
                  setForcedOpen(true);
                }}
              >
                Customise
              </Button>
            </>
          )}
        </div>
      </div>
    </section>
  );
}
