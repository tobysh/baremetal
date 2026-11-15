import "server-only";
import { connect } from "node:net";
import { io } from "next/cache";
import { ObjectId, type Filter } from "mongodb";
import { getDb } from "./db";
import { hmac } from "./crypto";
import { isSuspended } from "./suspension";
import type { UserDoc } from "./models";
import type { AuthEventType } from "./security";
import { budgetConfig, estimateCostMicro, tutorUsageOwner } from "./tutor-budget";
import { findExercise, getTracks, lessonPath } from "./curriculum";

const escapeRegex = (s: string) => s.replace(/[.*+?^${}()|[\]\\]/g, "\\$&");

// ---------- Audit trail ----------

export type AdminAction =
  | "user.create"
  | "user.reset_link"
  | "user.resend_verification"
  | "user.lock"
  | "user.unlock"
  | "user.ban"
  | "user.unban"
  | "user.revoke_sessions"
  | "user.revoke_session"
  | "user.delete"
  | "tutor.purge_answer";

export const ADMIN_ACTION_LABEL: Record<AdminAction, string> = {
  "user.create": "Created account",
  "user.reset_link": "Sent password reset link",
  "user.resend_verification": "Resent verification email",
  "user.lock": "Locked account",
  "user.unlock": "Unlocked account",
  "user.ban": "Banned account",
  "user.unban": "Lifted ban",
  "user.revoke_sessions": "Signed out all devices",
  "user.revoke_session": "Signed out one device",
  "user.delete": "Deleted account",
  "tutor.purge_answer": "Purged cached tutor answer",
};

/** Every admin action is recorded: who, what, which account (by id only) and an optional note. Kept one year (TTL index in db.ts). */
export async function logAdminAction(adminId: string, action: AdminAction, targetId?: string, note?: string) {
  await (await getDb()).collection("admin_audit").insertOne({ adminId, action, ...(targetId ? { targetId } : {}), ...(note ? { note: note.slice(0, 300) } : {}), ts: new Date() });
}

export type AuditRow = { ts: Date; admin: string; action: AdminAction; label: string; targetId: string | null; target: string | null; note: string | null };

export async function listAdminAudit({ targetId, limit = 50 }: { targetId?: string; limit?: number } = {}): Promise<AuditRow[]> {
  await io();
  return auditRows(targetId ? { targetId } : {}, 0, limit);
}

async function auditRows(query: Filter<Record<string, unknown>>, skip: number, limit: number): Promise<AuditRow[]> {
  const db = await getDb();
  const rows = await db.collection("admin_audit").find(query).sort({ ts: -1 }).skip(skip).limit(limit).toArray();
  const ids = new Set<string>();
  for (const r of rows) {
    if (r.adminId) ids.add(String(r.adminId));
    if (r.targetId) ids.add(String(r.targetId));
  }
  const objectIds = [...ids].filter((i) => ObjectId.isValid(i)).map((i) => new ObjectId(i));
  const users = await db.collection<UserDoc>("users").find({ _id: { $in: objectIds } }, { projection: { name: 1, username: 1 } }).toArray();
  const label = new Map(users.map((u) => [u._id.toHexString(), u.username ?? u.name]));
  return rows.map((r) => ({
    ts: r.ts,
    admin: label.get(String(r.adminId)) ?? "removed admin",
    action: r.action,
    label: ADMIN_ACTION_LABEL[r.action as AdminAction] ?? String(r.action),
    targetId: r.targetId ?? null,
    target: r.targetId ? (label.get(String(r.targetId)) ?? "deleted account") : null,
    note: r.note ?? null,
  }));
}

export const ADMIN_ACTIONS = Object.keys(ADMIN_ACTION_LABEL) as AdminAction[];
const AUDIT_PAGE = 25;

/** Paginated admin audit trail filtered by acting admin (user id) and action. Values are matched exactly. */
export async function listAdminAuditPage(adminId: string, action: string, page: number): Promise<{ rows: AuditRow[]; page: number; pages: number; total: number; admins: { id: string; name: string }[] }> {
  await io();
  const col = (await getDb()).collection("admin_audit");
  const query: Filter<Record<string, unknown>> = {};
  if (/^[0-9a-f]{24}$/.test(adminId)) query.adminId = adminId;
  if ((ADMIN_ACTIONS as string[]).includes(action)) query.action = action;
  const total = await col.countDocuments(query);
  const pages = Math.max(1, Math.ceil(total / AUDIT_PAGE));
  const current = Math.min(Math.max(1, Math.floor(page) || 1), pages);
  const rows = await auditRows(query, (current - 1) * AUDIT_PAGE, AUDIT_PAGE);
  const adminIds = (await col.distinct("adminId")).map(String).filter((i) => ObjectId.isValid(i));
  const users = await (await getDb()).collection<UserDoc>("users").find({ _id: { $in: adminIds.map((i) => new ObjectId(i)) } }, { projection: { name: 1, username: 1 } }).toArray();
  const admins = users.map((u) => ({ id: u._id.toHexString(), name: u.username ?? u.name })).sort((x, y) => x.name.localeCompare(y.name));
  return { rows, page: current, pages, total, admins };
}

// ---------- Users ----------

export type UserFilter = "all" | "suspended" | "unverified" | "admins" | "no2fa" | "active7" | "neverSignedIn";
export const USER_FILTERS: { value: UserFilter; label: string }[] = [
  { value: "all", label: "All" },
  { value: "suspended", label: "Locked or banned" },
  { value: "unverified", label: "Unverified" },
  { value: "admins", label: "Admins" },
  { value: "no2fa", label: "2FA off" },
  { value: "active7", label: "Active in last 7 days" },
  { value: "neverSignedIn", label: "Never signed in" },
];
export const USER_SORTS = ["xp", "joined", "lastLogin"] as const;
export type UserSort = (typeof USER_SORTS)[number];
export type SortDir = "asc" | "desc";
const SIGN_IN_TYPES = ["login_success", "backup_code_used"];
export type AdminUserListRow = AdminUserRow & { lastLogin: Date | null };
export const PAGE_SIZE = 25;

/** Matches accounts whose lock or ban is still in force (an expired timed lock is not). */
const inForce = (): Filter<UserDoc> => ({ $or: [{ "suspension.kind": "banned" }, { suspension: { $exists: true }, "suspension.until": { $exists: false } }, { "suspension.until": { $gt: new Date() } }] });

export type AdminUserRow = {
  id: string;
  email: string;
  name: string;
  username: string | null;
  role: UserDoc["role"];
  emailVerified: boolean;
  twoFactor: boolean;
  xp: number;
  createdAt: Date;
  status: "active" | "locked" | "banned";
};

const statusOf = (u: UserDoc): AdminUserRow["status"] => (isSuspended(u) ? u.suspension!.kind : "active");
const toRow = (u: UserDoc): AdminUserRow => ({
  id: u._id.toHexString(),
  email: u.email,
  name: u.name,
  username: u.username ?? null,
  role: u.role,
  emailVerified: u.emailVerified !== false,
  twoFactor: !!u.twoFactor?.enabled,
  xp: u.xp ?? 0,
  createdAt: u.createdAt,
  status: statusOf(u),
});

export async function searchUsers(
  q: string,
  filter: UserFilter,
  page: number,
  sort: UserSort = "joined",
  dir: SortDir = "desc",
): Promise<{ users: AdminUserListRow[]; total: number; page: number; pages: number }> {
  await io();
  const db = await getDb();
  const query: Filter<UserDoc> = {};
  const term = q.trim().slice(0, 100);
  if (term) {
    const rx = new RegExp(escapeRegex(term), "i");
    query.$or = [{ email: rx }, { username: rx }, { name: rx }, ...(/^[0-9a-f]{24}$/i.test(term) ? [{ _id: new ObjectId(term) }] : [])];
  }
  if (filter === "suspended") query.$and = [inForce()];
  if (filter === "unverified") query.emailVerified = false;
  if (filter === "admins") query.role = "admin";
  if (filter === "no2fa") query["twoFactor.enabled"] = { $ne: true };
  if (filter === "active7" || filter === "neverSignedIn") {
    // Sign-ins live in auth_events (userId as a string, 90-day retention).
    const since = filter === "active7" ? { ts: { $gte: new Date(Date.now() - 7 * 86400_000) } } : {};
    const ids = (await db.collection("auth_events").distinct("userId", { type: { $in: SIGN_IN_TYPES }, ...since })) as string[];
    const oids = ids.filter((x) => typeof x === "string" && /^[0-9a-f]{24}$/i.test(x)).map((x) => new ObjectId(x));
    query._id = filter === "active7" ? { $in: oids } : { $nin: oids };
  }
  const col = db.collection<UserDoc>("users");
  const total = await col.countDocuments(query);
  const pages = Math.max(1, Math.ceil(total / PAGE_SIZE));
  const current = Math.min(Math.max(1, page), pages);
  const dirN = dir === "asc" ? 1 : -1;
  const skip = (current - 1) * PAGE_SIZE;
  const lastLoginFor = async (ids: ObjectId[]) => {
    const rows = await db
      .collection("auth_events")
      .aggregate<{ _id: string; ts: Date }>([
        { $match: { userId: { $in: ids.map((i) => i.toHexString()) }, type: { $in: SIGN_IN_TYPES } } },
        { $group: { _id: "$userId", ts: { $max: "$ts" } } },
      ])
      .toArray();
    return new Map(rows.map((r) => [r._id, r.ts]));
  };
  let docs: UserDoc[];
  let logins: Map<string, Date>;
  if (sort === "lastLogin") {
    // Needs the sign-in time before paging, so join it in the database. Users without one sort as oldest.
    const rows = await col
      .aggregate<UserDoc & { lastLoginAt?: Date }>([
        { $match: query },
        { $lookup: { from: "auth_events", let: { id: { $toString: "$_id" } }, pipeline: [{ $match: { $expr: { $eq: ["$userId", "$$id"] }, type: { $in: SIGN_IN_TYPES } } }, { $group: { _id: null, ts: { $max: "$ts" } } }], as: "ll" } },
        { $set: { lastLoginAt: { $first: "$ll.ts" } } },
        { $project: { ll: 0 } },
        { $sort: { lastLoginAt: dirN, _id: dirN } },
        { $skip: skip },
        { $limit: PAGE_SIZE },
      ])
      .toArray();
    docs = rows;
    logins = new Map(rows.filter((r) => r.lastLoginAt).map((r) => [r._id.toHexString(), r.lastLoginAt!]));
  } else {
    const key = sort === "xp" ? "xp" : "createdAt";
    docs = await col.find(query).sort({ [key]: dirN, _id: dirN }).skip(skip).limit(PAGE_SIZE).toArray();
    logins = docs.length ? await lastLoginFor(docs.map((d) => d._id)) : new Map();
  }
  return { users: docs.map((d) => ({ ...toRow(d), lastLogin: logins.get(d._id.toHexString()) ?? null })), total, page: current, pages };
}

export type AdminSessionRow = { id: string; createdAt: Date; lastSeenAt: Date; device: string };
export type AdminProgress = {
  tracks: { slug: string; title: string; passed: number; total: number }[];
  recent: { title: string; href: string; status: "started" | "passed"; at: Date }[];
  tutorQuestions: number;
};

/** A short "Browser on OS" label from a stored user agent; the raw string is never shown. */
export function summariseUserAgent(ua: string | undefined): string {
  if (!ua) return "Unknown device";
  const browser = /Edg\//.test(ua) ? "Edge" : /OPR\/|Opera/.test(ua) ? "Opera" : /Firefox\//.test(ua) ? "Firefox" : /Chrome\/|CriOS/.test(ua) ? "Chrome" : /Safari\//.test(ua) ? "Safari" : "Other browser";
  const os = /Android/.test(ua) ? "Android" : /iPhone|iPad|iOS/.test(ua) ? "iOS" : /Windows/.test(ua) ? "Windows" : /Mac OS X|Macintosh/.test(ua) ? "macOS" : /CrOS/.test(ua) ? "ChromeOS" : /Linux/.test(ua) ? "Linux" : "unknown OS";
  return `${browser} on ${os}`;
}

export type AdminUserDetail = AdminUserRow & {
  experience: string;
  passed: number;
  lastLogin: Date | null;
  activeSessions: number;
  sessions: AdminSessionRow[];
  profilePublic: boolean;
  progress: AdminProgress;
  pendingReset: boolean;
  pendingVerification: boolean;
  suspension: { kind: "locked" | "banned"; reason: string | null; at: Date; until: Date | null } | null;
  events: { ts: Date; type: string; reason: string | null; ipHash: string }[];
};

export async function getAdminUser(id: string): Promise<AdminUserDetail | null> {
  await io();
  if (!ObjectId.isValid(id) || id.length !== 24) return null;
  const db = await getDb();
  const user = await db.collection<UserDoc>("users").findOne({ _id: new ObjectId(id) });
  if (!user) return null;
  const now = new Date();
  const liveSessions = { userId: user._id, expiresAt: { $gt: now }, retired: { $ne: true } };
  const [passed, lastLogin, sessionDocs, reset, verification, events, progressDocs, tutorQuestions] = await Promise.all([
    db.collection("progress").countDocuments({ userId: user._id, status: "passed" }),
    db.collection("auth_events").find({ userId: id, type: { $in: ["login_success", "backup_code_used"] } }).sort({ ts: -1 }).limit(1).toArray(),
    db.collection("sessions").find(liveSessions, { projection: { createdAt: 1, lastSeenAt: 1, userAgent: 1 } }).sort({ lastSeenAt: -1 }).limit(50).toArray(),
    db.collection("password_resets").countDocuments({ userId: user._id, expiresAt: { $gt: now } }),
    db.collection("email_verifications").countDocuments({ userId: user._id, expiresAt: { $gt: now } }),
    db.collection("auth_events").find({ $or: [{ userId: id }, { email: user.email }] }).sort({ ts: -1 }).limit(20).toArray(),
    db.collection("progress").find({ userId: user._id }, { projection: { exerciseId: 1, trackSlug: 1, status: 1, updatedAt: 1, passedAt: 1 } }).toArray(),
    db.collection("tutor_usage").countDocuments({ owner: tutorUsageOwner(id) }),
  ]);
  const s = user.suspension;
  const lessonDocs = progressDocs.filter((p) => !String(p.exerciseId).includes("#"));
  const tracks = getTracks().flatMap((t) => {
    const mine = lessonDocs.filter((p) => p.trackSlug === t.slug);
    return mine.length ? [{ slug: t.slug, title: t.title, passed: mine.filter((p) => p.status === "passed").length, total: t.lessons.length }] : [];
  });
  const recent = progressDocs
    .map((p) => ({ p, at: (p.status === "passed" ? p.passedAt : null) ?? p.updatedAt }))
    .filter((x) => x.at instanceof Date)
    .sort((a, b) => b.at.getTime() - a.at.getTime())
    .flatMap(({ p, at }) => {
      const hit = findExercise(String(p.exerciseId));
      if (!hit) return [];
      return [{ title: hit.isChallenge ? `${hit.lesson.title} (challenge)` : hit.lesson.title, href: lessonPath(hit.track.slug, hit.lesson.slug), status: p.status as "started" | "passed", at }];
    })
    .slice(0, 5);
  return {
    ...toRow(user),
    experience: user.experience,
    passed,
    lastLogin: lastLogin[0]?.ts ?? null,
    activeSessions: sessionDocs.length,
    profilePublic: user.profilePublic !== false,
    sessions: sessionDocs.map((d) => ({ id: d._id.toHexString(), createdAt: d.createdAt, lastSeenAt: d.lastSeenAt ?? d.createdAt, device: summariseUserAgent(d.userAgent) })),
    progress: { tracks, recent, tutorQuestions },
    pendingReset: reset > 0,
    pendingVerification: verification > 0,
    suspension: s ? { kind: s.kind, reason: s.reason ?? null, at: s.at, until: s.until ?? null } : null,
    events: events.map((e) => ({ ts: e.ts, type: String(e.type), reason: e.reason ?? null, ipHash: hmac("log-ip", String(e.ip ?? "")).slice(0, 10) })),
  };
}

// ---------- Security log ----------

export const AUTH_EVENT_TYPES: AuthEventType[] = [
  "login_failed", "login_locked", "login_blocked", "login_success", "otp_failed", "signup", "password_reset_requested", "password_reset",
  "password_changed", "2fa_enabled", "2fa_disabled", "backup_code_used", "account_deleted", "rate_limited", "captcha_failed",
];

export type AuthSeverity = "critical" | "warning" | "info" | "ok";
export const AUTH_EVENT_INFO: Record<string, { label: string; severity: AuthSeverity }> = {
  login_failed: { label: "Failed sign-in", severity: "warning" },
  login_locked: { label: "Account locked by failures", severity: "critical" },
  login_blocked: { label: "Sign-in blocked", severity: "critical" },
  login_success: { label: "Signed in", severity: "ok" },
  otp_failed: { label: "Wrong 2FA code", severity: "warning" },
  signup: { label: "New account", severity: "info" },
  password_reset_requested: { label: "Password reset requested", severity: "info" },
  password_reset: { label: "Password reset", severity: "info" },
  password_changed: { label: "Password changed", severity: "info" },
  "2fa_enabled": { label: "2FA enabled", severity: "ok" },
  "2fa_disabled": { label: "2FA disabled", severity: "warning" },
  backup_code_used: { label: "Backup code used", severity: "warning" },
  account_deleted: { label: "Account deleted", severity: "info" },
  rate_limited: { label: "Rate limited", severity: "warning" },
  captcha_failed: { label: "CAPTCHA failed", severity: "warning" },
};

export type AuthEventRow = {
  ts: Date; type: string; email: string | null; userId: string | null; reason: string | null; ipHash: string; userAgent: string;
  /** Distinct accounts (emails or user ids) this hash touched within the filtered window. */
  ipAccounts: number;
};
export type AuthEventFilter = { type?: string; q?: string; ip?: string; from?: string; to?: string };
export type AuthSummary = {
  failed: number; locked: number;
  topEmails: { email: string; count: number }[];
  topHashes: { ipHash: string; count: number; accounts: number }[];
};

const ipHash = (ip: unknown) => hmac("log-ip", String(ip ?? "")).slice(0, 10);
const parseDay = (v: string | undefined, endOfDay: boolean): Date | null => {
  if (!v || !/^\d{4}-\d{2}-\d{2}$/.test(v)) return null;
  const d = new Date(`${v}T${endOfDay ? "23:59:59.999" : "00:00:00.000"}Z`);
  return Number.isNaN(d.getTime()) ? null : d;
};

/** Raw IPs stay in the database; admins see a keyed hash, enough to tell whether events share an address. */
export async function listAuthEvents(f: AuthEventFilter, page: number): Promise<{ rows: AuthEventRow[]; page: number; pages: number; total: number; summary: AuthSummary }> {
  await io();
  const col = (await getDb()).collection("auth_events");
  const base: Filter<Record<string, unknown>> = {};
  const from = parseDay(f.from, false);
  const to = parseDay(f.to, true);
  if (from || to) base.ts = { ...(from ? { $gte: from } : {}), ...(to ? { $lte: to } : {}) };
  const term = (f.q ?? "").trim().slice(0, 100);
  if (term) base.email = new RegExp(escapeRegex(term), "i");
  if (f.ip && /^[0-9a-f]{6,16}$/.test(f.ip)) {
    // Hashes are keyed, so resolve them to the stored addresses and match those exactly.
    const ips = await col.distinct("ip", base.ts ? { ts: base.ts } : {});
    base.ip = { $in: ips.filter((ip) => ipHash(ip) === f.ip) };
  }
  const query: Filter<Record<string, unknown>> = { ...base };
  if (f.type && (AUTH_EVENT_TYPES as string[]).includes(f.type)) query.type = f.type;

  const total = await col.countDocuments(query);
  const pages = Math.max(1, Math.ceil(total / 50));
  const current = Math.min(Math.max(1, page), pages);
  const [docs, byType, byEmail, byIp] = await Promise.all([
    col.find(query).sort({ ts: -1 }).skip((current - 1) * 50).limit(50).toArray(),
    col.aggregate<{ _id: string; n: number }>([{ $match: base }, { $group: { _id: "$type", n: { $sum: 1 } } }]).toArray(),
    col.aggregate<{ _id: string; n: number }>([
      { $match: { ...base, type: { $in: ["login_failed", "login_locked", "otp_failed"] }, email: { $type: "string" } } },
      { $group: { _id: "$email", n: { $sum: 1 } } }, { $sort: { n: -1 } }, { $limit: 5 },
    ]).toArray(),
    col.aggregate<{ _id: unknown; failed: number; accounts: number }>([
      { $match: base },
      { $group: { _id: "$ip", failed: { $sum: { $cond: [{ $in: ["$type", ["login_failed", "login_locked", "otp_failed"]] }, 1, 0] } }, ids: { $addToSet: { $ifNull: ["$email", { $toString: "$userId" }] } } } },
      { $project: { failed: 1, accounts: { $size: "$ids" } } },
    ]).toArray(),
  ]);
  const hashes = new Map<string, { count: number; accounts: number }>();
  for (const r of byIp) {
    const h = ipHash(r._id);
    const cur = hashes.get(h) ?? { count: 0, accounts: 0 };
    hashes.set(h, { count: cur.count + r.failed, accounts: cur.accounts + r.accounts });
  }
  const n = (t: string) => byType.find((b) => b._id === t)?.n ?? 0;
  return {
    total,
    page: current,
    pages,
    summary: {
      failed: n("login_failed"),
      locked: n("login_locked"),
      topEmails: byEmail.map((b) => ({ email: b._id, count: b.n })),
      topHashes: [...hashes].filter(([, v]) => v.count > 0).sort((a, b) => b[1].count - a[1].count).slice(0, 5).map(([h, v]) => ({ ipHash: h, count: v.count, accounts: v.accounts })),
    },
    rows: docs.map((e) => {
      const h = ipHash(e.ip);
      return {
        ts: e.ts, type: String(e.type), email: e.email ?? null, userId: e.userId ?? null, reason: e.reason ?? null,
        ipHash: h, userAgent: String(e.userAgent ?? "").slice(0, 80), ipAccounts: hashes.get(h)?.accounts ?? 0,
      };
    }),
  };
}

// ---------- System health ----------

export type Health = {
  db: { ok: boolean; pingMs: number | null };
  smtp: { configured: boolean; host: string | null; from: string | null };
  queue: { pending: number; retrying: number; oldestMinutes: number | null };
  tutor: { provider: string; configured: boolean };
  runner: { mode: "socket" | "local"; reachable: boolean | null };
  secret: { ok: boolean };
  accounts: { total: number; unverified: number; suspended: number; adminsWithout2fa: number };
};

function probeSocket(path: string): Promise<boolean> {
  return new Promise((resolve) => {
    const sock = connect(path);
    const done = (ok: boolean) => { sock.destroy(); resolve(ok); };
    sock.setTimeout(1000, () => done(false));
    sock.once("connect", () => done(true));
    sock.once("error", () => done(false));
  });
}

export async function getSystemHealth(): Promise<Health> {
  await io();
  const started = Date.now();
  const db = await getDb();
  let dbOk = true;
  await db.command({ ping: 1 }).catch(() => { dbOk = false; });
  const pingMs = dbOk ? Date.now() - started : null;

  let host: string | null = null;
  try { host = process.env.SMTP_URL ? new URL(process.env.SMTP_URL).hostname : null; } catch { host = "(invalid SMTP_URL)"; }

  const queue = db.collection("email_queue");
  const [pending, retrying, oldest, total, unverified, suspended, admins] = await Promise.all([
    queue.countDocuments({}),
    queue.countDocuments({ attempts: { $gt: 1 } }),
    queue.find({}, { projection: { createdAt: 1 } }).sort({ createdAt: 1 }).limit(1).toArray(),
    db.collection("users").countDocuments({}),
    db.collection("users").countDocuments({ emailVerified: false }),
    db.collection<UserDoc>("users").countDocuments(inForce()),
    db.collection<UserDoc>("users").find({ role: "admin" }, { projection: { twoFactor: 1 } }).toArray(),
  ]);

  const provider = process.env.TUTOR_PROVIDER === "ollama" ? "ollama" : "claude";
  const socket = process.env.RUNNER_SOCKET;
  return {
    db: { ok: dbOk, pingMs },
    smtp: { configured: !!process.env.SMTP_URL, host, from: process.env.EMAIL_FROM ?? null },
    queue: { pending, retrying, oldestMinutes: oldest[0] ? Math.round((Date.now() - oldest[0].createdAt.getTime()) / 60_000) : null },
    tutor: { provider, configured: provider === "ollama" ? true : !!process.env.ANTHROPIC_API_KEY },
    runner: { mode: socket ? "socket" : "local", reachable: socket ? await probeSocket(socket) : null },
    secret: { ok: (process.env.AUTH_SECRET?.length ?? 0) >= 32 },
    accounts: { total, unverified, suspended, adminsWithout2fa: admins.filter((a) => !a.twoFactor?.enabled).length },
  };
}

// ---------- Tutor cost ----------

export type TutorCost = {
  days: number;
  totalUsd: number;
  requests: number;
  cacheHits: number;
  perDay: { day: string; requests: number; usd: number }[];
  /** Days (per user) on which the daily allowance was used up, and days the site-wide ceiling was hit. Spend records are kept 60 days. */
  userCeilingHits: number;
  globalCeilingDays: number;
  ceilings: { userDayUsd: number; globalDayUsd: number };
};

export async function getTutorCost(days: number): Promise<TutorCost> {
  await io();
  const db = await getDb();
  const since = new Date(Date.now() - days * 86_400_000);
  const cfg = budgetConfig();
  const [groups, userHits, globalHits] = await Promise.all([
    db.collection("tutor_usage").aggregate([
      { $match: { ts: { $gte: since } } },
      { $group: {
        _id: { day: { $dateToString: { format: "%Y-%m-%d", date: "$ts" } }, provider: "$provider" },
        requests: { $sum: 1 },
        hits: { $sum: { $cond: ["$responseCacheHit", 1, 0] } },
        inputTokens: { $sum: { $ifNull: ["$inputTokens", 0] } },
        outputTokens: { $sum: { $ifNull: ["$outputTokens", 0] } },
        cacheReadTokens: { $sum: { $ifNull: ["$cacheReadTokens", 0] } },
        cacheWriteTokens: { $sum: { $ifNull: ["$cacheWriteTokens", 0] } },
      } },
    ]).toArray(),
    cfg.userDayUsd > 0 ? db.collection("tutor_spend").countDocuments({ key: { $regex: "^u:" }, createdAt: { $gte: since }, micro: { $gte: cfg.userDayUsd * 1e6 } }) : 0,
    cfg.globalDayUsd > 0 ? db.collection("tutor_spend").countDocuments({ key: { $regex: "^g:" }, createdAt: { $gte: since }, micro: { $gte: cfg.globalDayUsd * 1e6 } }) : 0,
  ]);
  const byDay = new Map<string, { requests: number; micro: number }>();
  let requests = 0, cacheHits = 0, micro = 0;
  for (const g of groups) {
    const cost = estimateCostMicro({ inputTokens: g.inputTokens, outputTokens: g.outputTokens, cacheReadTokens: g.cacheReadTokens, cacheWriteTokens: g.cacheWriteTokens, provider: g._id.provider });
    const d = byDay.get(g._id.day) ?? { requests: 0, micro: 0 };
    d.requests += g.requests; d.micro += cost;
    byDay.set(g._id.day, d);
    requests += g.requests; cacheHits += g.hits; micro += cost;
  }
  return {
    days, totalUsd: micro / 1e6, requests, cacheHits,
    perDay: [...byDay.entries()].sort(([a], [b]) => a.localeCompare(b)).map(([day, d]) => ({ day, requests: d.requests, usd: d.micro / 1e6 })),
    userCeilingHits: userHits, globalCeilingDays: globalHits,
    ceilings: { userDayUsd: cfg.userDayUsd, globalDayUsd: cfg.globalDayUsd },
  };
}
