// Rebuilds every user's XP ledger and total from the lessons, challenges and practice they've passed and the current
// lesson difficulties. Run after changing a lesson's difficulty, or once to backfill the ledger and the
// review schedule for accounts that passed lessons before they existed:
//   npm run backup-db -- --verify
//   npm run recompute-xp -- --confirm-backup
// It refuses to run without --confirm-backup, which states that a verified backup exists and the site is stopped.
// Each user is computed completely in memory first (a pending pass with no recorded decision aborts that user before any
// write). The writes then run in one transaction when the database is a replica set. Standalone MongoDB has no
// transactions, so the writes are ordered to be idempotent instead (progress, ledger rows upserted by their unique key and
// stale rows removed, the review schedule, and the user's total last): a crash mid-user leaves a stale total, and running
// the script again repairs it.
// Lessons whose XP was withheld by the pass-pacing rule (xpWithheld) stay at 0. Review them with: npm run flagged-accounts
import { MongoClient, ObjectId, type ClientSession, type Document } from "mongodb";
import { getTracks } from "../src/lib/curriculum/index";
import { challengeXpFor, isChallengeId, isPracticeId, practiceIdOf, practiceXpFor, xpFor } from "../src/lib/gamification";
import { REVIEW_INTERVALS_DAYS } from "../src/lib/progress-constants";

const xpByExercise = new Map<string, number>();
for (const t of getTracks()) {
  for (const l of t.lessons) {
    xpByExercise.set(l.exercise.id, xpFor(l.difficulty));
    if (l.challenge) xpByExercise.set(l.challenge.exercise.id, challengeXpFor(l.difficulty));
    if (l.practice) xpByExercise.set(practiceIdOf(l.exercise.id), practiceXpFor(l.difficulty));
  }
}
if (!process.argv.includes("--confirm-backup")) {
  console.error("recompute-xp rewrites every user's XP and ledger. Take a verified backup (npm run backup-db -- --verify), stop the site, then run: npm run recompute-xp -- --confirm-backup");
  process.exit(2);
}
const client = await new MongoClient(process.env.MONGODB_URI ?? "mongodb://127.0.0.1:27017").connect();
const db = client.db(process.env.MONGODB_DB ?? "baremetal");
const hello = await client.db("admin").command({ hello: 1 });
const transactional = Boolean(hello.setName) || hello.msg === "isdbgrid";
console.log(transactional ? "Replica set detected: each user is written in one transaction." : "Standalone MongoDB: no transactions, each user is written as ordered idempotent steps (re-run after a crash).");

type Ledger = { userId: ObjectId; source: "practice" | "challenge" | "lesson"; ref: string; xp: number; at: Date };
type Plan = { xp: number; progress: { _id: ObjectId; set: Document; unset?: Document }[]; ledger: Ledger[]; reviews: { exerciseId: string; first: Date }[] };

async function planUser(user: Document): Promise<Plan> {
  const passed = await db.collection("progress").find({ userId: user._id, status: "passed" }, { projection: { exerciseId: 1, xpWithheld: 1, xpPending: 1, passedAt: 1 } }).toArray();
  const plan: Plan = { xp: 0, progress: [], ledger: [], reviews: [] };
  for (const p of passed) {
    const set: Document = {};
    const unset: Document = {};
    if (p.xpPending) {
      const decision = (user.xpQuota ?? []).find((entry: { ref?: string }) => entry.ref === p.exerciseId);
      if (!decision) throw new Error(`Pending XP for ${user._id}/${p.exerciseId} has no recorded decision. Review it before recomputing; no award is assumed.`);
      p.xpWithheld = decision.flag != null;
      if (decision.flag) { set.xpWithheld = true; set.xpFlag = decision.flag; }
      unset.xpPending = "";
    }
    const award = p.xpWithheld ? 0 : (xpByExercise.get(p.exerciseId) ?? 0);
    set.xpAwarded = award;
    plan.xp += award;
    plan.progress.push({ _id: p._id, set, unset });
    if (award > 0) plan.ledger.push({ userId: user._id, source: isPracticeId(p.exerciseId) ? "practice" : isChallengeId(p.exerciseId) ? "challenge" : "lesson", ref: p.exerciseId, xp: award, at: p.passedAt ?? new Date(0) });
    // Make sure every passed lesson has a place in the review schedule.
    if (!isChallengeId(p.exerciseId) && !isPracticeId(p.exerciseId) && xpByExercise.has(p.exerciseId)) plan.reviews.push({ exerciseId: p.exerciseId, first: p.passedAt ?? new Date() });
  }
  return plan;
}

async function applyPlan(user: Document, plan: Plan, session?: ClientSession) {
  const opts = session ? { session } : {};
  if (plan.progress.length) {
    await db.collection("progress").bulkWrite(
      plan.progress.map((p) => ({ updateOne: { filter: { _id: p._id }, update: { $set: p.set, ...(Object.keys(p.unset ?? {}).length ? { $unset: p.unset } : {}) } } })),
      { ordered: true, ...opts },
    );
  }
  const keep = plan.ledger.map((e) => ({ source: e.source, ref: e.ref }));
  const events = db.collection("xp_events");
  await events.deleteMany(keep.length ? { userId: user._id, $nor: keep } : { userId: user._id }, opts);
  if (plan.ledger.length) {
    await events.bulkWrite(plan.ledger.map((e) => ({ updateOne: { filter: { userId: user._id, source: e.source, ref: e.ref }, update: { $set: { xp: e.xp, at: e.at } }, upsert: true } })), { ordered: true, ...opts });
  }
  if (plan.reviews.length) {
    await db.collection("reviews").bulkWrite(plan.reviews.map((r) => ({
      updateOne: {
        filter: { userId: user._id, exerciseId: r.exerciseId },
        update: { $setOnInsert: { dueAt: new Date(r.first.getTime() + REVIEW_INTERVALS_DAYS[0] * 86_400_000), interval: REVIEW_INTERVALS_DAYS[0], lastAt: r.first, count: 0 } },
        upsert: true,
      },
    })), { ordered: true, ...opts });
  }
  // The total goes last: until it is written, a crash leaves a visibly stale value that a re-run repairs.
  await db.collection("users").updateOne({ _id: user._id }, { $set: { xp: plan.xp }, $unset: { xpQuota: "", xpDecision: "" } }, opts);
}

let changed = 0;
for await (const user of db.collection("users").find({}, { projection: { _id: 1, xp: 1, xpQuota: 1 } })) {
  const plan = await planUser(user);
  if (transactional) {
    const session = client.startSession();
    try { await session.withTransaction(() => applyPlan(user, plan, session)); } finally { await session.endSession(); }
  } else {
    await applyPlan(user, plan);
  }
  if ((user.xp ?? 0) !== plan.xp) changed++;
}
console.log(`XP and ledger recomputed; ${changed} account(s) updated.`);
await client.close();
