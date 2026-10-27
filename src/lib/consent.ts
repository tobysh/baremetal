// Cookie consent record, shared by the banner (client) and server checks.
export const CONSENT_COOKIE = "bm_consent";
export const CONSENT_VERSION = 1;
export const CONSENT_MAX_AGE = 60 * 60 * 24 * 182; // ~6 months, then ask again

export type Consent = { v: number; analytics: boolean; at: string };

export function parseConsent(raw: string | undefined | null): Consent | null {
  if (!raw) return null;
  try {
    const c = JSON.parse(decodeURIComponent(raw)) as Consent;
    return c.v === CONSENT_VERSION && typeof c.analytics === "boolean" ? c : null;
  } catch {
    return null;
  }
}

export const serializeConsent = (analytics: boolean): string =>
  encodeURIComponent(JSON.stringify({ v: CONSENT_VERSION, analytics, at: new Date().toISOString() } satisfies Consent));
