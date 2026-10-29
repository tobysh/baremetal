// Dollar ceilings for tutor spend, per user and per UTC day, estimated from the
// token usage the API reports. Spend is kept in micro-dollars in Mongo
// (`tutor_spend`), one document per scope and day.
//
// Env (all optional; a ceiling of 0 disables that check):
//   TUTOR_BUDGET_USER_DAY_USD    per-user daily ceiling   (default 0.25)
//   TUTOR_BUDGET_GLOBAL_DAY_USD  site-wide daily ceiling  (default 20)
//   TUTOR_PRICE_IN_PER_MTOK / TUTOR_PRICE_OUT_PER_MTOK   USD per million tokens (default 1 / 5)
//   TUTOR_PRICE_CACHE_READ_MULT / TUTOR_PRICE_CACHE_WRITE_MULT  multiples of the input price (default 0.1 / 1.25)

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
  await Promise.all([
    col.updateOne({ key: `u:${userId}:${day}` }, { $inc: { micro }, $setOnInsert: { createdAt: at } }, { upsert: true }),
    col.updateOne({ key: `g:${day}` }, { $inc: { micro }, $setOnInsert: { createdAt: at } }, { upsert: true }),
  ]);
  return micro;
}
