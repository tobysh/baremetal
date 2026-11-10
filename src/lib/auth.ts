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
import { isSuspended } from "./suspension";
import { PENDING_2FA_COOKIE, SECURE_COOKIES, SESSION_COOKIE } from "./session-cookie";
import { killUser as killShells } from "./shell/kill";

const scrypt = promisify(scryptCb) as (pw: string, salt: Buffer, len: number, opts: object) => Promise<Buffer>;
// New hashes: N=2^17, r=8 (128 MiB, a few hundred ms). The parameters are stored in the hash so they can be raised
// later; hashes without them (`scrypt$salt$key`) were made with N=2^15 and still verify, then are rehashed on login.
const SCRYPT_N = 131072;
const SCRYPT_R = 8;
const SCRYPT_P = 1;
const LEGACY_PARAMS = { N: 32768, r: 8, p: 1 };
const SCRYPT_MAXMEM = 256 * 1024 * 1024;
const scryptOpts = (p: { N: number; r: number; p: number }) => ({ ...p, maxmem: SCRYPT_MAXMEM });

export { SESSION_COOKIE, PENDING_2FA_COOKIE };
const SESSION_DAYS = 30;
/** A session unused for this long is dead, even if it has not reached its expiry. */
const IDLE_MS = 14 * 86_400_000;
const ADMIN_SESSION_MS = 12 * 3_600_000;
const ADMIN_IDLE_MS = 2 * 3_600_000;
/** The token is replaced when it is older than this; the previous one stays valid briefly for parallel requests. */
const ROTATE_AFTER_MS = 24 * 3_600_000;
const ROTATE_GRACE_MS = 60_000;
const TOUCH_EVERY_MS = 5 * 60_000;
const PENDING_2FA_MINUTES = 10;

const cookieBase = {
  httpOnly: true,
  secure: SECURE_COOKIES,
  sameSite: "strict" as const,
  path: "/",
};

export async function hashPassword(password: string): Promise<string> {
  const salt = randomBytes(16);
  const key = await scrypt(password.normalize("NFKC"), salt, 64, scryptOpts({ N: SCRYPT_N, r: SCRYPT_R, p: SCRYPT_P }));
  return `scrypt$N=${SCRYPT_N},r=${SCRYPT_R},p=${SCRYPT_P}$${salt.toString("base64")}$${key.toString("base64")}`;
}

function parseHash(stored: string) {
  const parts = stored.split("$");
  if (parts[0] !== "scrypt") return null;
  let params = LEGACY_PARAMS;
  let rest = parts.slice(1);
  if (rest.length === 3) {
    const m = /^N=(\d+),r=(\d+),p=(\d+)$/.exec(rest[0]);
    if (!m) return null;
    params = { N: Number(m[1]), r: Number(m[2]), p: Number(m[3]) };
    rest = rest.slice(1);
  }
  if (rest.length !== 2 || !rest[0] || !rest[1]) return null;
  // Bound the cost so a corrupted stored value can't make a login allocate gigabytes.
  const { N, r, p } = params;
  if (N < 1024 || N > 1 << 18 || (N & (N - 1)) !== 0 || r < 1 || r > 16 || p < 1 || p > 4) return null;
  if (128 * N * r > SCRYPT_MAXMEM) return null;
  return { params, salt: Buffer.from(rest[0], "base64"), key: Buffer.from(rest[1], "base64") };
}

export async function verifyPassword(password: string, stored: string): Promise<boolean> {
  const h = parseHash(stored);
  if (!h) return false;
  const actual = await scrypt(password.normalize("NFKC"), h.salt, h.key.length, scryptOpts(h.params));
  // A legacy hash is cheaper to check than the dummy hash used for unknown emails; pay the difference so latency
  // doesn't reveal which accounts exist.
  if (h.params.N < SCRYPT_N) await scrypt(password.normalize("NFKC"), h.salt, h.key.length, scryptOpts({ N: SCRYPT_N, r: SCRYPT_R, p: SCRYPT_P }));
  return timingSafeEqual(actual, h.key);
}

/** True when a stored hash uses weaker parameters than the current ones (call after a successful verify). */
export function passwordNeedsRehash(stored: string): boolean {
  const h = parseHash(stored);
  return !h || h.params.N < SCRYPT_N || h.params.r < SCRYPT_R || h.params.p < SCRYPT_P;
}

/** A real hash to compare against when the email is unknown, so timing doesn't reveal which accounts exist. */
let dummyHash: Promise<string> | null = null;
export const getDummyHash = () => (dummyHash ??= hashPassword(randomToken()));

// ---------- Sessions ----------

export async function createSession(user: Pick<UserDoc, "_id" | "sessionVersion" | "role">, userAgent?: string) {
  const token = randomToken(32);
  const now = new Date();
  const lifetime = user.role === "admin" ? ADMIN_SESSION_MS : SESSION_DAYS * 86_400_000;
  const expiresAt = new Date(now.getTime() + lifetime);
  const db = await getDb();
  const jar = await cookies();
  // A fresh login replaces whatever session this browser still holds, so old tokens don't pile up.
  const old = jar.get(SESSION_COOKIE)?.value;
  if (old) await db.collection("sessions").deleteOne({ tokenHash: sha256(old) });
  await db.collection<Omit<SessionDoc, "_id">>("sessions").insertOne({
    tokenHash: sha256(token),
    userId: user._id,
    sessionVersion: user.sessionVersion ?? 0,
    createdAt: now,
    lastSeenAt: now,
    expiresAt,
    userAgent: userAgent?.slice(0, 200),
  });
  jar.set(SESSION_COOKIE, token, { ...cookieBase, expires: expiresAt });
}

/**
 * Replaces the session token once it is older than a day. Needs a context that may set cookies (a server action or
 * route handler), so it is called from those rather than from `loadUser`. The old record survives for a minute so
 * parallel requests that still carry the old token are not signed out. Only one of two racing calls wins.
 */
export async function rotateSessionIfDue(): Promise<void> {
  const jar = await cookies();
  const token = jar.get(SESSION_COOKIE)?.value;
  if (!token) return;
  const db = await getDb();
  const sessions = db.collection<SessionDoc>("sessions");
  const now = new Date();
  const cut = new Date(now.getTime() - ROTATE_AFTER_MS);
  const before = await sessions.findOneAndUpdate(
    { tokenHash: sha256(token), retired: { $ne: true }, expiresAt: { $gt: now }, $or: [{ rotatedAt: { $lt: cut } }, { rotatedAt: { $exists: false }, createdAt: { $lt: cut } }] },
    { $set: { retired: true, expiresAt: new Date(now.getTime() + ROTATE_GRACE_MS) } },
    { returnDocument: "before" },
  );
  if (!before) return;
  const next = randomToken(32);
  const { _id, ...rest } = before;
  void _id;
  await db.collection<Omit<SessionDoc, "_id">>("sessions").insertOne({ ...rest, tokenHash: sha256(next), lastSeenAt: now, rotatedAt: now, retired: false });
  jar.set(SESSION_COOKIE, next, { ...cookieBase, expires: before.expiresAt });
}

export async function destroySession() {
  const jar = await cookies();
  const token = jar.get(SESSION_COOKIE)?.value;
  if (token) {
    const db = await getDb();
    const sessions = db.collection<SessionDoc>("sessions");
    const own = await sessions.findOne({ tokenHash: sha256(token) });
    // Rotation leaves the predecessor valid for a minute; remove it with the current record.
    if (own) {
      await sessions.deleteMany({ userId: own.userId, createdAt: own.createdAt });
      // Signing out ends the user's interactive shells in this process (other processes notice on their heartbeat).
      killShells(own.userId.toHexString());
    }
  }
  jar.delete(SESSION_COOKIE);
}

/**
 * Invalidate every session for a user (password reset, 2FA change, admin lock, ban and sign-out-everywhere).
 * Pass `keepCurrent` to re-issue a session for the browser making the change. Ends the user's interactive shells too.
 */
export async function revokeAllSessions(userId: ObjectId, keepCurrent?: { userAgent?: string }) {
  const db = await getDb();
  const user = await db
    .collection<UserDoc>("users")
    .findOneAndUpdate({ _id: userId }, { $inc: { sessionVersion: 1 } }, { returnDocument: "after" });
  await db.collection("sessions").deleteMany({ userId });
  killShells(userId.toHexString());
  if (keepCurrent && user) await createSession(user, keepCurrent.userAgent);
}

/** Hash of this request's session token (the `tokenHash` key of its record), or null without a cookie. */
export async function currentSessionHash(): Promise<string | null> {
  const token = await currentSessionToken();
  return token ? sha256(token) : null;
}

/**
 * For a long-lived connection bound to one session (the interactive shell): the token hash of that session's live
 * record, following a rotation to its successor, or null once the session was signed out, revoked or expired, or the
 * account was suspended, deleted or had its sessions invalidated. Applies the same rules as `loadUser`, without
 * touching `lastSeenAt`.
 */
export async function liveSessionHash(tokenHash: string): Promise<string | null> {
  const db = await getDb();
  const now = new Date();
  const sessions = db.collection<SessionDoc>("sessions");
  let session = await sessions.findOne({ tokenHash });
  if (!session) return null;
  if (session.retired || session.expiresAt <= now) {
    // Rotation keeps userId and createdAt; the successor is the family's record that is not retired.
    session = await sessions.findOne({ userId: session.userId, createdAt: session.createdAt, retired: { $ne: true }, expiresAt: { $gt: now } });
    if (!session) return null;
  }
  const user = await db.collection<UserDoc>("users").findOne({ _id: session.userId }, { projection: { sessionVersion: 1, suspension: 1, role: 1 } });
  if (!user || (user.sessionVersion ?? 0) !== session.sessionVersion || isSuspended(user)) return null;
  const admin = user.role === "admin";
  const lastSeen = (session.lastSeenAt ?? session.createdAt).getTime();
  if (now.getTime() - lastSeen > (admin ? ADMIN_IDLE_MS : IDLE_MS)) return null;
  if (admin && now.getTime() - session.createdAt.getTime() > ADMIN_SESSION_MS) return null;
  return session.tokenHash;
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
  const now = new Date();
  const sessions = db.collection<SessionDoc>("sessions");
  const session = await sessions.findOne({ tokenHash: sha256(token), expiresAt: { $gt: now } });
  if (!session) return null;
  const user = await db.collection<UserDoc>("users").findOne({ _id: session.userId });
  if (!user || (user.sessionVersion ?? 0) !== session.sessionVersion || isSuspended(user)) return null;
  // Admin limits apply by current role, so an older, longer session of a newly promoted admin is cut short too.
  const admin = user.role === "admin";
  const lastSeen = (session.lastSeenAt ?? session.createdAt).getTime();
  if (now.getTime() - lastSeen > (admin ? ADMIN_IDLE_MS : IDLE_MS)) return null;
  if (admin && now.getTime() - session.createdAt.getTime() > ADMIN_SESSION_MS) return null;
  if (now.getTime() - lastSeen > TOUCH_EVERY_MS) await sessions.updateOne({ _id: session._id }, { $set: { lastSeenAt: now } });
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
