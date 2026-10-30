import "server-only";
import { goalOf } from "./rhythm";
import { randomBytes, scrypt as scryptCb, timingSafeEqual } from "node:crypto";
import { promisify } from "node:util";
import { io } from "next/cache";
import { cookies } from "next/headers";
import { redirect } from "next/navigation";
import { cache } from "react";
import { ObjectId } from "mongodb";
import { getDb } from "./db";
import { randomToken, sha256 } from "./crypto";
import type { ExperienceLevel, SessionDoc, UserDoc } from "./models";

const scrypt = promisify(scryptCb) as (pw: string, salt: Buffer, len: number, opts: object) => Promise<Buffer>;
// N=2^15, r=8: ~32 MiB and ~50ms per hash on a desktop CPU.
const SCRYPT_OPTS = { N: 32768, r: 8, p: 1, maxmem: 64 * 1024 * 1024 };

export const SESSION_COOKIE = "bm_session";
export const PENDING_2FA_COOKIE = "bm_2fa_pending";
const SESSION_DAYS = 30;
const PENDING_2FA_MINUTES = 10;

const cookieBase = {
  httpOnly: true,
  secure: process.env.NODE_ENV === "production",
  sameSite: "strict" as const,
  path: "/",
};

export async function hashPassword(password: string): Promise<string> {
  const salt = randomBytes(16);
  const key = await scrypt(password.normalize("NFKC"), salt, 64, SCRYPT_OPTS);
  return `scrypt$${salt.toString("base64")}$${key.toString("base64")}`;
}

export async function verifyPassword(password: string, stored: string): Promise<boolean> {
  const [scheme, saltB64, keyB64] = stored.split("$");
  if (scheme !== "scrypt" || !saltB64 || !keyB64) return false;
  const expected = Buffer.from(keyB64, "base64");
  const actual = await scrypt(password.normalize("NFKC"), Buffer.from(saltB64, "base64"), expected.length, SCRYPT_OPTS);
  return timingSafeEqual(actual, expected);
}

/** A real hash to compare against when the email is unknown, so timing doesn't reveal which accounts exist. */
let dummyHash: Promise<string> | null = null;
export const getDummyHash = () => (dummyHash ??= hashPassword(randomToken()));

// ---------- Sessions ----------

export async function createSession(user: Pick<UserDoc, "_id" | "sessionVersion">, userAgent?: string) {
  const token = randomToken(32);
  const now = new Date();
  const expiresAt = new Date(now.getTime() + SESSION_DAYS * 86_400_000);
  const db = await getDb();
  await db.collection<Omit<SessionDoc, "_id">>("sessions").insertOne({
    tokenHash: sha256(token),
    userId: user._id,
    sessionVersion: user.sessionVersion ?? 0,
    createdAt: now,
    expiresAt,
    userAgent: userAgent?.slice(0, 200),
  });
  (await cookies()).set(SESSION_COOKIE, token, { ...cookieBase, expires: expiresAt });
}

export async function destroySession() {
  const jar = await cookies();
  const token = jar.get(SESSION_COOKIE)?.value;
  if (token) {
    const db = await getDb();
    await db.collection("sessions").deleteOne({ tokenHash: sha256(token) });
  }
  jar.delete(SESSION_COOKIE);
}

/**
 * Invalidate every session for a user (password reset, 2FA change, deletion).
 * Pass `keepCurrent` to re-issue a session for the browser making the change.
 */
export async function revokeAllSessions(userId: ObjectId, keepCurrent?: { userAgent?: string }) {
  const db = await getDb();
  const user = await db
    .collection<UserDoc>("users")
    .findOneAndUpdate({ _id: userId }, { $inc: { sessionVersion: 1 } }, { returnDocument: "after" });
  await db.collection("sessions").deleteMany({ userId });
  if (keepCurrent && user) await createSession(user, keepCurrent.userAgent);
}

async function currentSessionToken() {
  return (await cookies()).get(SESSION_COOKIE)?.value;
}

export type CurrentUser = {
  id: string;
  email: string;
  name: string;
  experience: ExperienceLevel;
  role: UserDoc["role"];
  twoFactorEnabled: boolean;
  username: string | null;
  xp: number;
  profilePublic: boolean;
  /** IANA zone, or null until the browser has reported one. */
  timezone: string | null;
  /** Lessons per week the learner is aiming for (0 = none). */
  weeklyGoal: number;
};

async function loadUser(): Promise<UserDoc | null> {
  const token = await currentSessionToken();
  if (!token) return null;
  // Session expiry compares against the current time, which must never be
  // captured into a prerendered shell.
  await io();
  const db = await getDb();
  const session = await db
    .collection<SessionDoc>("sessions")
    .findOne({ tokenHash: sha256(token), expiresAt: { $gt: new Date() } });
  if (!session) return null;
  const user = await db.collection<UserDoc>("users").findOne({ _id: session.userId });
  if (!user || (user.sessionVersion ?? 0) !== session.sessionVersion) return null;
  return user;
}

/** Full user document for server-side use (never pass to the client). Deduplicated per request. */
export const getUserDoc = cache(loadUser);

/** The signed-in user, or null. Safe to pass to client components. */
export const getCurrentUser = cache(async (): Promise<CurrentUser | null> => {
  const user = await getUserDoc();
  if (!user) return null;
  return {
    id: user._id.toHexString(),
    email: user.email,
    name: user.name,
    experience: user.experience,
    role: user.role,
    twoFactorEnabled: !!user.twoFactor?.enabled,
    username: user.username ?? null,
    xp: user.xp ?? 0,
    profilePublic: user.profilePublic !== false,
    timezone: user.timezone ?? null,
    weeklyGoal: goalOf(user.weeklyGoal),
  };
});

export async function requireUser(returnTo?: string): Promise<CurrentUser> {
  const user = await getCurrentUser();
  if (!user) redirect(returnTo ? `/login?next=${encodeURIComponent(returnTo)}` : "/login");
  return user;
}

/**
 * Server-side admin gate for pages and server actions. Admins must have 2FA
 * enabled; until they do, they are sent to set it up.
 */
export async function requireAdmin(): Promise<CurrentUser> {
  const user = await requireUser("/admin");
  if (user.role !== "admin") redirect("/dashboard");
  if (!user.twoFactorEnabled) redirect("/settings/security?required=admin");
  return user;
}

/** Same gate for route handlers: returns a user or null instead of redirecting. */
export async function adminFromRequest(): Promise<CurrentUser | null> {
  const user = await getCurrentUser();
  return user && user.role === "admin" && user.twoFactorEnabled ? user : null;
}

// ---------- Pending second factor ----------

export async function startPending2fa(userId: ObjectId, next: string, sessionVersion: number) {
  const token = randomToken(32);
  const expiresAt = new Date(Date.now() + PENDING_2FA_MINUTES * 60_000);
  const db = await getDb();
  await db.collection("pending_2fa").insertOne({ tokenHash: sha256(token), userId, next, sessionVersion, attempts: 0, expiresAt });
  (await cookies()).set(PENDING_2FA_COOKIE, token, { ...cookieBase, expires: expiresAt });
}

export type Pending2fa = { _id: ObjectId; userId: ObjectId; next: string; sessionVersion: number; attempts: number };

export async function getPending2fa(): Promise<Pending2fa | null> {
  const token = (await cookies()).get(PENDING_2FA_COOKIE)?.value;
  if (!token) return null;
  const db = await getDb();
  return db
    .collection<Pending2fa & { tokenHash: string; expiresAt: Date }>("pending_2fa")
    .findOne({ tokenHash: sha256(token), expiresAt: { $gt: new Date() } });
}

export async function clearPending2fa() {
  const jar = await cookies();
  const token = jar.get(PENDING_2FA_COOKIE)?.value;
  if (token) {
    const db = await getDb();
    await db.collection("pending_2fa").deleteOne({ tokenHash: sha256(token) });
  }
  jar.delete(PENDING_2FA_COOKIE);
}

export { safeNext } from "./redirects";

export const toObjectId = (id: string) => new ObjectId(id);
