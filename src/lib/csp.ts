/**
 * Content Security Policy for HTML pages. Shared by the proxy (which sets it per request),
 * next.config.ts (the fallback on responses the proxy does not see) and scripts/security/csp-check.mts.
 *
 * Why nonce plus hashes: with Cache Components every page is a prerendered static shell that is
 * served as-is and then resumed per request. Scripts in the resumed part (Flight data, the Turbopack
 * bootstrap, React's streaming scripts) carry the request's nonce, which Next reads from the
 * Content-Security-Policy request header the proxy sets. Scripts inside the static shell were rendered
 * at build time with no request and so have no nonce; they are a few fixed snippets (the theme bootstrap
 * and React's reveal runtime) and are allowed by their SHA-256 hash, collected from the built shells by
 * `inlineScriptHashes` in the proxy. External chunks come from 'self'. No 'strict-dynamic': the shell's
 * <script src> tags have no nonce, so it would block them. See docs/security.md ("Content Security Policy").
 */
import { createHash } from "node:crypto";

export type CspOptions = {
  /** Per-request nonce; omitted for the static fallback and for development. */
  nonce?: string;
  /** 'sha256-…' sources for the inline scripts in the prerendered shells. */
  scriptHashes?: readonly string[];
  dev?: boolean;
};

/** `'sha256-<base64>'` CSP source for an inline script body, exactly as it appears between the tags. */
export function scriptHash(body: string): string {
  return `'sha256-${createHash("sha256").update(body, "utf8").digest("base64")}'`;
}

/** Executable inline <script> bodies in an HTML document (external, JSON and other data blocks are skipped). */
export function inlineScripts(html: string): string[] {
  const out: string[] = [];
  for (const m of html.matchAll(/<script\b([^>]*)>([\s\S]*?)<\/script>/gi)) {
    const attrs = m[1];
    if (/\ssrc\s*=/i.test(attrs)) continue;
    const type = /\stype\s*=\s*"([^"]*)"/i.exec(attrs)?.[1]?.trim().toLowerCase();
    if (type && !["text/javascript", "application/javascript", "module"].includes(type)) continue;
    if (m[2].length) out.push(m[2]);
  }
  return out;
}

export function contentSecurityPolicy({ nonce, scriptHashes = [], dev = false }: CspOptions = {}): string {
  // Development renders everything per request but React and Next use eval and unnonced scripts for
  // HMR and the error overlay, so `next dev` keeps the relaxed policy. In production, with no nonce and
  // no hashes this is `script-src 'self'`: no inline script at all (the fallback for non-page responses).
  const script = dev
    ? "script-src 'self' 'unsafe-inline' 'unsafe-eval'"
    : ["script-src 'self'", ...(nonce ? [`'nonce-${nonce}'`] : []), ...scriptHashes].join(" ");
  return [
    "default-src 'self'",
    script,
    // Style attributes from React `style` props and CodeMirror's injected <style> elements need this.
    "style-src 'self' 'unsafe-inline'",
    "img-src 'self' data: blob:",
    "font-src 'self'",
    "connect-src 'self'",
    "worker-src 'self'",
    "manifest-src 'self'",
    "object-src 'none'",
    "base-uri 'self'",
    "form-action 'self'",
    "frame-ancestors 'none'",
    ...(dev ? [] : ["upgrade-insecure-requests"]),
  ].join("; ");
}

/** The pre-migration policy, used only if no prerendered shells can be read (see the proxy). */
export function legacyContentSecurityPolicy(): string {
  return contentSecurityPolicy().replace("script-src 'self'", "script-src 'self' 'unsafe-inline'");
}
