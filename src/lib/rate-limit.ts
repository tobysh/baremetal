import "server-only";
import { randomBytes } from "node:crypto";
import { getDb } from "./db";

/**
 * Fixed-window rate limiter backed by MongoDB, so limits survive dev-server
 * reloads and apply across processes. Returns true when the call is allowed.
 */
export async function rateLimit(key: string, limit: number, windowSeconds: number): Promise<boolean> {
  return (await rateLimitHit(key, limit, windowSeconds)).ok;
}

export type RateLimitResult = { ok: boolean; remaining: number; limit: number; resetAt: Date };

/** Counts one request against `key` and reports what's left in the window. */
export async function rateLimitHit(key: string, limit: number, windowSeconds: number): Promise<RateLimitResult> {
  const db = await getDb();
  const now = new Date();
  const windowStart = Math.floor(now.getTime() / (windowSeconds * 1000));
  const docKey = `${key}:${windowStart}`;
  const res = await db.collection<{ key: string; count: number; expiresAt: Date }>("rate_limits").findOneAndUpdate(
    { key: docKey },
    {
      $inc: { count: 1 },
      $setOnInsert: { expiresAt: new Date((windowStart + 1) * windowSeconds * 1000) },
    },
    { upsert: true, returnDocument: "after" },
  );
  const count = res?.count ?? 1;
  return { ok: count <= limit, remaining: Math.max(0, limit - count), limit, resetAt: new Date((windowStart + 1) * windowSeconds * 1000) };
}

/**
 * The client's IP for rate limiting. X-Forwarded-For is a list that each proxy
 * appends to, and its left end is whatever the client sent, so it can't be
 * trusted. We read the entry added by our own reverse proxy instead:
 * TRUSTED_PROXY_HOPS=1 (default) means "the last entry", 2 means
 * "second-from-last" (e.g. CDN in front of nginx), and so on.
 *
 * Next.js only fills this header in itself when it's missing, so in production
 * the app must run behind a proxy that sets it (nginx:
 * `proxy_set_header X-Forwarded-For $remote_addr;`). The proxy must overwrite
 * the header, not append, and production refuses to boot without
 * TRUSTED_PROXY_HOPS (see src/instrumentation.ts). Per-account limits and
 * lockouts don't depend on the IP, so they hold even if this is misconfigured.
 */
export function clientIp(headers: Headers): string {
  const hops = Math.max(1, Number(process.env.TRUSTED_PROXY_HOPS ?? 1) || 1);
  const chain = (headers.get("x-forwarded-for") ?? "")
    .split(",")
    .map((s) => s.trim())
    .filter(Boolean);
  // A chain shorter than the configured hop count means the proxy contract is
  // broken (or the request skipped the proxy). Never fall back to chain[0]: that
  // entry is client-controlled.
  return chain[chain.length - hops] ?? UNKNOWN_IP;
}

/**
 * Bucket for requests whose IP can't be determined. It is per-process, so a
 * missing header can't become one allowance shared with (and depleted by)
 * other users, and limits keyed on it are quartered.
 */
export const UNKNOWN_IP = `unknown-${process.pid}-${randomBytes(8).toString("hex")}`;

/** Divides a per-IP limit for the unknown bucket; pass-through otherwise. */
export function ipLimit(ip: string, limit: number): number {
  return ip === UNKNOWN_IP ? Math.max(1, Math.floor(limit / 4)) : limit;
}
