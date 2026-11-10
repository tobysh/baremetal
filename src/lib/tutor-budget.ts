// Dollar ceilings for tutor spend, per user and per UTC day, estimated from the
// token usage the API reports. Spend is kept in micro-dollars in Mongo
// (`tutor_spend`), one document per scope and day.
//
// Env (all optional; a ceiling of 0 disables that check):
//   TUTOR_BUDGET_USER_DAY_USD    per-user daily ceiling   (default 0.25)
//   TUTOR_BUDGET_GLOBAL_DAY_USD  site-wide daily ceiling  (default 20)
//   TUTOR_MODEL (src/lib/tutor.ts) selects the Claude model; the prices below are for the default, claude-haiku-5-5, so update them with it.
//   TUTOR_PRICE_IN_PER_MTOK / TUTOR_PRICE_OUT_PER_MTOK   USD per million tokens (default 1 / 5)
//   TUTOR_PRICE_CACHE_READ_MULT / TUTOR_PRICE_CACHE_WRITE_MULT  multiples of the input price (default 0.1 / 1.25)

import { ObjectId, type Db } from "mongodb";
import { hmac } from "./crypto";

export type Usage = { inputTokens: number; outputTokens: number; cacheReadTokens: number; cacheWriteTokens: number; provider?: string };

const num = (name: string, fallback: number) => {
  const raw = process.env[name];
  const n = raw === undefined || raw.trim() === "" ? NaN : Number(raw);
  return Number.isFinite(n) && n >= 0 ? n : fallback;
};

export const budgetConfig = () => ({
  userDayUsd: num("TUTOR_BUDGET_USER_DAY_USD", 0.25),
  globalDayUsd: num("TUTOR_BUDGET_GLOBAL_DAY_USD", 20),
  inPerMTok: num("TUTOR_PRICE_IN_PER_MTOK", 1),
  outPerMTok: num("TUTOR_PRICE_OUT_PER_MTOK", 5),
  cacheReadMult: num("TUTOR_PRICE_CACHE_READ_MULT", 0.1),
  cacheWriteMult: num("TUTOR_PRICE_CACHE_WRITE_MULT", 1.25),
});

/** Estimated cost in micro-dollars (1e-6 USD). Local models cost nothing. */
export function estimateCostMicro(u: Usage): number {
  if (u.provider === "ollama") return 0;
  const c = budgetConfig();
  // $/MTok equals micro-dollars per token.
  return Math.ceil(u.inputTokens * c.inPerMTok + u.cacheReadTokens * c.inPerMTok * c.cacheReadMult + u.cacheWriteTokens * c.inPerMTok * c.cacheWriteMult + u.outputTokens * c.outPerMTok);
}

export const budgetDay = (now = Date.now()) => new Date(now).toISOString().slice(0, 10);

type SpendCollection = {
  findOne(filter: { key: string }): Promise<{ micro?: number } | null>;
  updateOne(filter: { key: string }, update: { $inc: { micro: number }; $setOnInsert: { createdAt: Date } }, options: { upsert: boolean }): Promise<unknown>;
};
export type SpendDb = { collection(name: string): SpendCollection };
const COLLECTION = "tutor_spend";

export type BudgetVerdict = { ok: true } | { ok: false; scope: "user" | "global"; message: string };

/** Friendly message when a ceiling is hit; reset happens at 00:00 UTC. */
export async function checkBudget(db: SpendDb, userId: string, now = Date.now()): Promise<BudgetVerdict> {
  const c = budgetConfig();
  const day = budgetDay(now);
  const col = db.collection(COLLECTION);
  const hours = Math.max(1, Math.ceil((Date.parse(`${day}T00:00:00Z`) + 86_400_000 - now) / 3_600_000));
  if (c.userDayUsd > 0) {
    const spent = (await col.findOne({ key: `u:${userId}:${day}` }))?.micro ?? 0;
    if (spent >= c.userDayUsd * 1e6)
      return { ok: false, scope: "user", message: `You've used up today's tutor allowance. It refills in about ${hours} hour${hours === 1 ? "" : "s"} (midnight UTC). The hints and "What probably went wrong" notes are still free.` };
  }
  if (c.globalDayUsd > 0) {
    const spent = (await col.findOne({ key: `g:${day}` }))?.micro ?? 0;
    if (spent >= c.globalDayUsd * 1e6)
      return { ok: false, scope: "global", message: "The tutor has reached its daily capacity for everyone. It resets at midnight UTC; the hints are still available meanwhile." };
  }
  return { ok: true };
}

/** Adds a finished request's estimated cost to the user's and the site's day totals. Returns micro-dollars. */
export async function recordSpend(db: SpendDb, userId: string, usage: Usage, now = Date.now()): Promise<number> {
  const micro = estimateCostMicro(usage);
  if (micro <= 0) return 0;
  const day = budgetDay(now);
  const col = db.collection(COLLECTION);
  const at = new Date(now);
  // With the unique key index, two first-of-the-day upserts can race; the loser retries as a plain increment.
  const bump = async (key: string) => {
    try {
      await col.updateOne({ key }, { $inc: { micro }, $setOnInsert: { createdAt: at } }, { upsert: true });
    } catch (e) {
      if ((e as { code?: number })?.code !== 11000) throw e;
      await col.updateOne({ key }, { $inc: { micro }, $setOnInsert: { createdAt: at } }, { upsert: true });
    }
  };
  await Promise.all([bump(`u:${userId}:${day}`), bump(`g:${day}`)]);
  return micro;
}

type Reservation = {
  _id: ObjectId;
  userId: string;
  amount: number;
  keys: string[];
  state: "reserving" | "active" | "settling" | "settled";
  createdAt: Date;
  targetMicro?: number;
  recovery?: "refund-unstarted" | "charge-unknown";
  recoveryAttemptedAt?: Date;
  expiresAt?: Date;
};

// Longer than the provider deadlines: recovery must not race ordinary live calls.
const RESERVATION_STALE_MS = 60 * 60 * 1000;
const RESERVATION_RETENTION_MS = 60 * 24 * 60 * 60 * 1000;

async function reservationOwnerExists(db: Db, userId: string): Promise<boolean> {
  return ObjectId.isValid(userId) && !!await db.collection("users").findOne({ _id: new ObjectId(userId) }, { projection: { _id: 1 } });
}

/** Account deletion removes its journal and personal totals. A worker that read
 * the journal before deletion may finish afterwards: check after writing so it
 * cannot resurrect personal rows. Anonymous site totals remain accurate. */
async function cleanDeletedReservationOwner(db: Db, reservation: Reservation): Promise<void> {
  if (await reservationOwnerExists(db, reservation.userId)) return;
  for (const key of reservation.keys.filter((key) => key.startsWith("u:"))) await db.collection("tutor_spend").deleteOne({ key });
  await db.collection<Reservation>("tutor_reservations").deleteOne({ _id: reservation._id });
}

/** Apply the durable target to both scopes. The marker and total change in one
 * document write, so a lost acknowledgement or a partial multi-scope failure
 * can be replayed without charging/refunding twice. Zero markers also fence a
 * late hold write after an interrupted reservation was recovered. */
async function finishReservation(db: Db, reservation: Reservation, now: number): Promise<void> {
  if (reservation.state === "settled") { await cleanDeletedReservationOwner(db, reservation); return; }
  if (reservation.state !== "settling" || reservation.targetMicro === undefined) throw new Error("Tutor reservation has no settlement decision");
  const field = `reservations.${reservation._id.toHexString()}`;
  for (const key of reservation.keys) {
    const write = () => db.collection("tutor_spend").updateOne({ key }, [{ $set: {
      micro: { $add: [{ $ifNull: ["$micro", 0] }, { $subtract: [reservation.targetMicro, { $ifNull: [`$${field}`, 0] }] }] },
      [field]: reservation.targetMicro,
      createdAt: { $ifNull: ["$createdAt", reservation.createdAt] },
    } }], { upsert: true });
    try { await write(); } catch (error) {
      if ((error as { code?: number }).code !== 11000) throw error;
      await write(); // Concurrent first use of this day/scope won the unique-key race.
    }
  }
  // Keep the journal discoverable until owner cleanup succeeds too. Otherwise a
  // cleanup outage after marking settled would leave personal rows unrecoverable.
  await cleanDeletedReservationOwner(db, reservation);
  await db.collection<Reservation>("tutor_reservations").updateOne(
    { _id: reservation._id, state: "settling" },
    { $set: { state: "settled", expiresAt: new Date(now + RESERVATION_RETENTION_MS) } },
  );
}

async function decideSettlement(db: Db, id: ObjectId, targetMicro: number, now: number): Promise<void> {
  const journal = db.collection<Reservation>("tutor_reservations");
  // The first durable decision wins, including concurrent calls with different usage.
  await journal.updateOne({ _id: id, state: { $in: ["active", "reserving"] } }, { $set: { state: "settling", targetMicro } });
  const reservation = await journal.findOne({ _id: id });
  if (!reservation) throw new Error("Tutor reservation journal is missing");
  await finishReservation(db, reservation, now);
}

/** Hold the maximum estimated call cost so concurrent calls cannot all pass the
 * same balance check. A durable journal survives errors and process restarts. */
export async function reserveBudget(db: Db, userId: string, amount: number, now = Date.now()) {
  if (!Number.isSafeInteger(amount) || amount < 0) throw new Error("Invalid tutor reservation amount");
  if (amount === 0) return async (usage: Usage | null) => { void usage; }; // Local providers incur no cost.
  if (!await reservationOwnerExists(db, userId)) throw new Error("Tutor reservation owner no longer exists");
  const c = budgetConfig(), day = budgetDay(now);
  const scopes = ([[`u:${userId}:${day}`, c.userDayUsd], [`g:${day}`, c.globalDayUsd]] as const).filter(([, usd]) => usd > 0);
  if (scopes.some(([, usd]) => amount > Math.floor(usd * 1e6))) return null;
  const reservation: Reservation = { _id: new ObjectId(), userId, amount, keys: scopes.map(([key]) => key), state: "reserving", createdAt: new Date(now) };
  const journal = db.collection<Reservation>("tutor_reservations");
  await journal.insertOne(reservation);
  const field = `reservations.${reservation._id.toHexString()}`;
  try {
    for (const [key, usd] of scopes) {
      let granted = false;
      for (let retry = 0; retry < 2; retry++) {
        try {
          await db.collection("tutor_spend").updateOne(
            { key, micro: { $lte: Math.floor(usd * 1e6) - amount }, [field]: { $exists: false } },
            { $inc: { micro: amount }, $set: { [field]: amount }, $setOnInsert: { createdAt: new Date(now) } },
            { upsert: true },
          );
          granted = true;
          break;
        } catch (error) { if ((error as { code?: number }).code !== 11000) throw error; }
      }
      if (!granted) { await decideSettlement(db, reservation._id, 0, now); return null; }
    }
    const activated = await journal.updateOne({ _id: reservation._id, state: "reserving" }, { $set: { state: "active" } });
    if (!activated.modifiedCount) throw new Error("Tutor reservation expired before activation");
    if (!await reservationOwnerExists(db, userId)) throw new Error("Tutor reservation owner no longer exists");
  } catch (error) {
    // If this write fails too, the journal remains discoverable for recovery.
    await decideSettlement(db, reservation._id, 0, now).catch(() => {});
    // Deletion can remove the journal before an in-flight hold write finishes.
    // Activation then fails, but that late personal total still needs removal.
    await cleanDeletedReservationOwner(db, reservation).catch((cleanupError) => {
      console.error("[tutor-budget] reservation owner cleanup failed; inspect orphan personal totals if account deletion removed its journal", { event: "tutor_reservation_owner_cleanup_failed", error: cleanupError instanceof Error ? cleanupError.name : "Error" });
    });
    throw error;
  }
  return async (usage: Usage | null) => {
    const targetMicro = usage ? estimateCostMicro(usage) : 0;
    if (!Number.isSafeInteger(targetMicro) || targetMicro < 0) throw new Error("Invalid tutor settlement amount");
    await decideSettlement(db, reservation._id, targetMicro, Date.now());
  };
}

/** Replay partial settlements, refund unstarted abandoned reservations, and keep
 * the full hold for abandoned active calls whose upstream bill is unknown.
 * Unresolved journals have no TTL: persistence failures cannot erase recovery
 * intent. Call at startup and periodically; safe across concurrent workers. */
export async function reconcileTutorReservations(db: Db, now = Date.now(), limit = 100): Promise<{ settled: number; chargedConservatively: number; failed: number }> {
  const journal = db.collection<Reservation>("tutor_reservations");
  const rows = await journal.find({ $or: [
    { state: "settling" },
    { state: { $in: ["reserving", "active"] }, createdAt: { $lte: new Date(now - RESERVATION_STALE_MS) } },
  ] }).sort({ recoveryAttemptedAt: 1, createdAt: 1, _id: 1 }).limit(limit).toArray();
  let settled = 0, chargedConservatively = 0, failed = 0;
  for (const row of rows) {
    try {
      // Rotate repeatedly failing rows behind work that has not been attempted;
      // a bounded batch must not keep selecting the same broken oldest journal.
      await journal.updateOne({ _id: row._id }, { $set: { recoveryAttemptedAt: new Date(now) } });
      if (row.state !== "settling") {
        await journal.updateOne({ _id: row._id, state: row.state }, { $set: {
          state: "settling", targetMicro: row.state === "active" ? row.amount : 0,
          recovery: row.state === "active" ? "charge-unknown" : "refund-unstarted",
        } });
      }
      const current = await journal.findOne({ _id: row._id });
      if (!current) continue; // Account deletion can remove a journal read by this worker.
      await finishReservation(db, current, now);
      settled++;
      if (current.recovery === "charge-unknown") chargedConservatively++;
    } catch (error) {
      failed++;
      // Do not put account ids, scope keys or upstream diagnostics in this log.
      console.error("[tutor-budget] reservation recovery failed; journal retained for retry", { event: "tutor_reservation_recovery_failed", error: error instanceof Error ? error.name : "Error" });
    }
  }
  return { settled, chargedConservatively, failed };
}

// Per-question usage records (`tutor_usage`). Each new row carries `owner`, a keyed HMAC of the user id
// (never the id itself), so the account's rows can be exported and deleted. Rows written before this
// have no owner; they cannot be linked to anyone and age out through the 180-day TTL index.
type UsageCollection = {
  insertOne(doc: Record<string, unknown>): Promise<unknown>;
  deleteMany(filter: { owner: string }): Promise<unknown>;
  find(filter: { owner: string }, options?: { projection: Record<string, 0 | 1> }): { toArray(): Promise<Record<string, unknown>[]> };
};
type UsageDb = { collection(name: string): UsageCollection };

export const tutorUsageOwner = (userId: string) => hmac("tutor-usage-owner", userId);

export const recordTutorUsage = (db: UsageDb, userId: string, row: Record<string, unknown>, now = new Date()) =>
  db.collection("tutor_usage").insertOne({ ...row, owner: tutorUsageOwner(userId), ts: now });

export const deleteTutorUsage = (db: UsageDb, userId: string) => db.collection("tutor_usage").deleteMany({ owner: tutorUsageOwner(userId) });

export const exportTutorUsage = (db: UsageDb, userId: string) =>
  db.collection("tutor_usage").find({ owner: tutorUsageOwner(userId) }, { projection: { _id: 0, owner: 0 } }).toArray();
