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
      className="fixed inset-x-0 bottom-0 z-50 p-3 sm:bottom-4 sm:left-4 sm:right-auto sm:max-w-md sm:p-0"
    >
      <div className="rounded-xl border border-border bg-surface p-5 shadow-2xl shadow-black/30">
        <h2 className="text-base font-semibold">Cookies and analytics</h2>
        <p className="mt-2 text-sm text-muted">
          We use essential cookies to keep you signed in and remember this choice. With your permission we also count page visits with our own
          privacy-friendly analytics — no ads, no third parties, no tracking across days.{" "}
          <Link href="/cookies" className="text-accent underline underline-offset-2">
            Cookie Policy
          </Link>
        </p>
        {forcedOpen && (
          <div className="mt-4 space-y-3 rounded-lg border border-border p-3 text-sm">
            <label className="flex items-start gap-3">
              <input type="checkbox" checked disabled className="mt-1 h-4 w-4 accent-[var(--accent)]" />
              <span>
                <span className="font-medium">Essential</span> <span className="text-muted">— sign-in and security. Always on.</span>
              </span>
            </label>
            <label className="flex items-start gap-3">
              <input type="checkbox" checked={analytics} onChange={(e) => setAnalytics(e.target.checked)} className="mt-1 h-4 w-4 accent-[var(--accent)]" />
              <span>
                <span className="font-medium">Analytics</span> <span className="text-muted">— anonymous page counts to improve lessons.</span>
              </span>
            </label>
          </div>
        )}
        {/* Accept and reject are equally prominent, as GDPR guidance requires. */}
        <div className="mt-4 flex flex-wrap gap-2">
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
