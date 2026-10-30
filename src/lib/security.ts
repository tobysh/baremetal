import "server-only";
import { headers } from "next/headers";
import { getDb } from "./db";
import { hmac } from "./crypto";
import { clientIp, ipLimit, rateLimit } from "./rate-limit";

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
    console.warn(`[auth] ${type}`, { emailHash: details.email ? hmac("log-email", details.email) : undefined, ipHash: hmac("log-ip", ctx.ip), reason: details.reason });
  }
}

// A single pair bucket prevents a stranger at another address from locking out the owner.
// Reservation happens before scrypt or OTP verification, so concurrent guesses count.
const CAPTCHA_AFTER = 3;
const LOCK_AFTER = 5;
const MAX_LOCK_MINUTES = 60;
type FailureDoc = { key: string; count: number; inFlight?: number; lockedUntil?: Date; lastFailure: Date; expiresAt: Date };
const pairKey = (email: string, ip: string) => `pair:${hmac("login-pair", `${email.toLowerCase()}\0${ip}`)}`;
const otpKey = (userId: string) => `otp:${hmac("login-otp", userId)}`;
export type LoginGuard = { locked: boolean; retryAfterSeconds: number; captchaRequired: boolean };

async function guardKey(key: string): Promise<LoginGuard> {
  const doc = await (await getDb()).collection<FailureDoc>("login_failures").findOne({ key });
  const until = doc?.lockedUntil?.getTime() ?? 0;
  return { locked: until > Date.now(), retryAfterSeconds: Math.max(0, Math.ceil((until - Date.now()) / 1000)), captchaRequired: (doc?.count ?? 0) >= CAPTCHA_AFTER };
}
export const loginGuard = (email: string, ip: string) => guardKey(pairKey(email, ip));
export const otpGuard = (userId: string) => guardKey(otpKey(userId));

async function reserve(key: string): Promise<boolean> {
  const col = (await getDb()).collection<FailureDoc>("login_failures");
  const now = new Date();
  try {
    await col.updateOne({ key }, { $setOnInsert: { count: 0, inFlight: 0, lastFailure: now, expiresAt: new Date(now.getTime() + 86400_000) } }, { upsert: true });
  } catch (e) { if ((e as { code?: number }).code !== 11000) throw e; }
  // Filter and increment in one operation. The threshold-crossing attempt is allowed;
  // subsequent concurrent attempts see lockedUntil and cannot start verification.
  const doc = await col.findOneAndUpdate(
    { key, $or: [{ lockedUntil: { $exists: false } }, { lockedUntil: { $lte: now } }] },
    [ { $set: { count: { $add: ["$count", 1] }, inFlight: { $add: [{ $ifNull: ["$inFlight", 0] }, 1] }, lastFailure: now, expiresAt: new Date(now.getTime() + 86400_000) } },
      { $set: { lockedUntil: { $cond: [ { $gte: ["$count", LOCK_AFTER] }, { $dateAdd: { startDate: now, unit: "minute", amount: { $min: [MAX_LOCK_MINUTES, { $pow: [2, { $subtract: ["$count", LOCK_AFTER] }] }] } } }, "$$REMOVE" ] } } } ],
    { returnDocument: "after" },
  );
  return !!doc;
}
async function refund(key: string) {
  // Clear only the lock created by this reservation if it was the sole threshold attempt.
  await (await getDb()).collection<FailureDoc>("login_failures").updateOne({ key, count: { $gt: 0 } }, [
    { $set: { count: { $subtract: ["$count", 1] }, inFlight: { $max: [0, { $subtract: [{ $ifNull: ["$inFlight", 0] }, 1] }] } } },
    { $set: { lockedUntil: { $cond: [{ $lt: ["$count", LOCK_AFTER] }, "$$REMOVE", "$lockedUntil"] } } },
  ]);
}
async function finishFailure(key: string) {
  await (await getDb()).collection<FailureDoc>("login_failures").updateOne({ key }, [
    { $set: { inFlight: { $max: [0, { $subtract: [{ $ifNull: ["$inFlight", 0] }, 1] }] } } },
  ]);
}
export const reserveLoginAttempt = (email: string, ip: string) => reserve(pairKey(email, ip));
export const refundLoginAttempt = (email: string, ip: string) => refund(pairKey(email, ip));
export const finishLoginFailure = (email: string, ip: string) => finishFailure(pairKey(email, ip));
export const reserveOtpAttempt = (userId: string) => reserve(otpKey(userId));
export const refundOtpAttempt = (userId: string) => refund(otpKey(userId));
export const finishOtpFailure = (userId: string) => finishFailure(otpKey(userId));
export async function clearLoginFailures(email: string, ip: string, userId?: string) {
  const keys = [pairKey(email, ip)];
  if (userId) keys.push(otpKey(userId));
  // Preserve reservations made before a simultaneous successful login. If those
  // requests fail later, their guesses must still count against the next login.
  await (await getDb()).collection<FailureDoc>("login_failures").updateMany({ key: { $in: keys } }, [
    { $set: { count: { $ifNull: ["$inFlight", 0] } } },
    { $set: { lockedUntil: { $cond: [{ $lt: ["$count", LOCK_AFTER] }, "$$REMOVE", "$lockedUntil"] } } },
  ]);
}

/** IP-based limit for auth endpoints: `limit` requests per 15 minutes. */
export async function authRateLimit(endpoint: string, limit: number): Promise<boolean> {
  const { ip } = await requestContext();
  const ok = await rateLimit(`auth:${endpoint}:${hmac("rate-ip", ip)}`, ipLimit(ip, limit), 15 * 60);
  if (!ok) await logAuthEvent("rate_limited", { reason: endpoint });
  return ok;
}

export function formatWait(seconds: number): string {
  if (seconds < 90) return `${seconds} seconds`;
  return `${Math.ceil(seconds / 60)} minutes`;
}
