import "server-only";
import { connect } from "node:net";
import { io } from "next/cache";
import { ObjectId, type Filter } from "mongodb";
import { getDb } from "./db";
import { hmac } from "./crypto";
import { isSuspended } from "./suspension";
import type { UserDoc } from "./models";
import type { AuthEventType } from "./security";
import { budgetConfig, estimateCostMicro } from "./tutor-budget";

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
  const db = await getDb();
  const rows = await db.collection("admin_audit").find(targetId ? { targetId } : {}).sort({ ts: -1 }).limit(limit).toArray();
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

// ---------- Users ----------

export type UserFilter = "all" | "suspended" | "unverified" | "admins";
export const USER_FILTERS: { value: UserFilter; label: string }[] = [
  { value: "all", label: "All" },
  { value: "suspended", label: "Locked or banned" },
  { value: "unverified", label: "Unverified" },
  { value: "admins", label: "Admins" },
];
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

export async function searchUsers(q: string, filter: UserFilter, page: number): Promise<{ users: AdminUserRow[]; total: number; page: number; pages: number }> {
  await io();
  const query: Filter<UserDoc> = {};
  const term = q.trim().slice(0, 100);
  if (term) {
    const rx = new RegExp(escapeRegex(term), "i");
    query.$or = [{ email: rx }, { username: rx }, { name: rx }, ...(/^[0-9a-f]{24}$/i.test(term) ? [{ _id: new ObjectId(term) }] : [])];
  }
  if (filter === "suspended") query.$and = [inForce()];
  if (filter === "unverified") query.emailVerified = false;
  if (filter === "admins") query.role = "admin";
  const col = (await getDb()).collection<UserDoc>("users");
  const total = await col.countDocuments(query);
  const pages = Math.max(1, Math.ceil(total / PAGE_SIZE));
  const current = Math.min(Math.max(1, page), pages);
  const docs = await col.find(query).sort({ createdAt: -1 }).skip((current - 1) * PAGE_SIZE).limit(PAGE_SIZE).toArray();
  return { users: docs.map(toRow), total, page: current, pages };
}

export type AdminUserDetail = AdminUserRow & {
  experience: string;
  passed: number;
  lastLogin: Date | null;
  activeSessions: number;
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
  const [passed, lastLogin, activeSessions, reset, verification, events] = await Promise.all([
    db.collection("progress").countDocuments({ userId: user._id, status: "passed" }),
    db.collection("auth_events").find({ userId: id, type: { $in: ["login_success", "backup_code_used"] } }).sort({ ts: -1 }).limit(1).toArray(),
    db.collection("sessions").countDocuments({ userId: user._id, expiresAt: { $gt: now } }),
    db.collection("password_resets").countDocuments({ userId: user._id, expiresAt: { $gt: now } }),
    db.collection("email_verifications").countDocuments({ userId: user._id, expiresAt: { $gt: now } }),
    db.collection("auth_events").find({ $or: [{ userId: id }, { email: user.email }] }).sort({ ts: -1 }).limit(20).toArray(),
  ]);
  const s = user.suspension;
  return {
    ...toRow(user),
    experience: user.experience,
    passed,
    lastLogin: lastLogin[0]?.ts ?? null,
    activeSessions,
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

export type AuthEventRow = { ts: Date; type: string; email: string | null; userId: string | null; reason: string | null; ipHash: string; userAgent: string };

/** Raw IPs stay in the database; admins see a keyed hash, enough to tell whether events share an address. */
export async function listAuthEvents(type: string, q: string, page: number): Promise<{ rows: AuthEventRow[]; page: number; pages: number; total: number }> {
  await io();
  const query: Filter<Record<string, unknown>> = {};
  if ((AUTH_EVENT_TYPES as string[]).includes(type)) query.type = type;
  const term = q.trim().slice(0, 100);
  if (term) query.email = new RegExp(escapeRegex(term), "i");
  const col = (await getDb()).collection("auth_events");
  const total = await col.countDocuments(query);
  const pages = Math.max(1, Math.ceil(total / 50));
  const current = Math.min(Math.max(1, page), pages);
  const docs = await col.find(query).sort({ ts: -1 }).skip((current - 1) * 50).limit(50).toArray();
  return {
    total,
    page: current,
    pages,
    rows: docs.map((e) => ({
      ts: e.ts, type: String(e.type), email: e.email ?? null, userId: e.userId ?? null, reason: e.reason ?? null,
      ipHash: hmac("log-ip", String(e.ip ?? "")).slice(0, 10), userAgent: String(e.userAgent ?? "").slice(0, 80),
    })),
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
