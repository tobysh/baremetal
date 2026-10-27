import "server-only";
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
 * `proxy_set_header X-Forwarded-For $remote_addr;`). Per-account limits and
 * lockouts don't depend on the IP, so they hold even if this is misconfigured.
 */
export function clientIp(headers: Headers): string {
  const hops = Math.max(1, Number(process.env.TRUSTED_PROXY_HOPS ?? 1) || 1);
  const chain = (headers.get("x-forwarded-for") ?? "")
    .split(",")
    .map((s) => s.trim())
    .filter(Boolean);
  return chain[chain.length - hops] ?? chain[0] ?? "unknown";
}
