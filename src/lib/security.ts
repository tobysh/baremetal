import "server-only";
import { headers } from "next/headers";
import { getDb } from "./db";
import { sha256 } from "./crypto";
import { clientIp, rateLimit } from "./rate-limit";

export type AuthEventType =
  | "login_failed"
  | "login_locked"
  | "login_success"
  | "otp_failed"
  | "signup"
  | "password_reset_requested"
  | "password_reset"
  | "password_changed"
  | "2fa_enabled"
  | "2fa_disabled"
  | "backup_code_used"
  | "account_deleted"
  | "rate_limited"
  | "captcha_failed";

export async function requestContext() {
  const h = await headers();
  return { ip: clientIp(h), userAgent: (h.get("user-agent") ?? "").slice(0, 300) };
}

/** Security log, kept 90 days (TTL index in db.ts). */
export async function logAuthEvent(type: AuthEventType, details: { email?: string; userId?: string; reason?: string } = {}) {
  const ctx = await requestContext();
  const db = await getDb();
  await db.collection("auth_events").insertOne({ type, ...details, ...ctx, ts: new Date() });
  if (type.endsWith("_failed") || type === "login_locked" || type === "rate_limited") {
    console.warn(`[auth] ${type}`, { email: details.email, ip: ctx.ip, reason: details.reason });
  }
}

// ---------- Brute-force protection ----------
// Failures are tracked per email (whether or not the account exists, so
// lockouts don't reveal which emails are registered) and per IP.

const CAPTCHA_AFTER = 3;
const LOCK_AFTER = 5;
const MAX_LOCK_MINUTES = 60;

type FailureDoc = { key: string; count: number; lockedUntil?: Date; lastFailure: Date; expiresAt: Date };

const emailKey = (email: string) => `email:${sha256(email.toLowerCase())}`;
const ipKey = (ip: string) => `ip:${ip}`;

export type LoginGuard = { locked: boolean; retryAfterSeconds: number; captchaRequired: boolean };

export async function loginGuard(email: string, ip: string): Promise<LoginGuard> {
  const db = await getDb();
  const docs = await db
    .collection<FailureDoc>("login_failures")
    .find({ key: { $in: [emailKey(email), ipKey(ip)] } })
    .toArray();
  const now = Date.now();
  const lockedUntil = Math.max(0, ...docs.map((d) => d.lockedUntil?.getTime() ?? 0));
  const maxCount = Math.max(0, ...docs.map((d) => d.count));
  return {
    locked: lockedUntil > now,
    retryAfterSeconds: Math.max(0, Math.ceil((lockedUntil - now) / 1000)),
    captchaRequired: maxCount >= CAPTCHA_AFTER,
  };
}

/** Records a failure; locks with exponential backoff (1, 2, 4 … 60 min) from the 5th. */
export async function recordLoginFailure(email: string, ip: string) {
  const db = await getDb();
  const col = db.collection<FailureDoc>("login_failures");
  const now = new Date();
  for (const key of [emailKey(email), ipKey(ip)]) {
    const doc = await col.findOneAndUpdate(
      { key },
      { $inc: { count: 1 }, $set: { lastFailure: now, expiresAt: new Date(now.getTime() + 24 * 3600_000) } },
      { upsert: true, returnDocument: "after" },
    );
    if (doc && doc.count >= LOCK_AFTER) {
      const minutes = Math.min(MAX_LOCK_MINUTES, 2 ** (doc.count - LOCK_AFTER));
      await col.updateOne({ key }, { $set: { lockedUntil: new Date(now.getTime() + minutes * 60_000) } });
    }
  }
}

export async function clearLoginFailures(email: string, ip: string) {
  const db = await getDb();
  await db.collection("login_failures").deleteMany({ key: { $in: [emailKey(email), ipKey(ip)] } });
}

/** IP-based limit for auth endpoints: `limit` requests per 15 minutes. */
export async function authRateLimit(endpoint: string, limit: number): Promise<boolean> {
  const { ip } = await requestContext();
  const ok = await rateLimit(`auth:${endpoint}:${ip}`, limit, 15 * 60);
  if (!ok) await logAuthEvent("rate_limited", { reason: endpoint });
  return ok;
}

export function formatWait(seconds: number): string {
  if (seconds < 90) return `${seconds} seconds`;
  return `${Math.ceil(seconds / 60)} minutes`;
}
