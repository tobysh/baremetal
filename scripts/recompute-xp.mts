// Rebuilds every user's XP ledger and total from the lessons and challenges they've passed and the current
// lesson difficulties. Run after changing a lesson's difficulty, or once to backfill the ledger and the
// review schedule for accounts that passed lessons before they existed:
//   npm run recompute-xp
// Lessons whose XP was withheld by the pass-pacing rule (xpWithheld) stay at 0. Review them with: npm run flagged-accounts
import { MongoClient } from "mongodb";
import { getTracks } from "../src/lib/curriculum/index";
import { challengeXpFor, isChallengeId, xpFor } from "../src/lib/gamification";
import { REVIEW_INTERVALS_DAYS } from "../src/lib/progress-constants";

const xpByExercise = new Map<string, number>();
for (const t of getTracks()) {
  for (const l of t.lessons) {
    xpByExercise.set(l.exercise.id, xpFor(l.difficulty));
    if (l.challenge) xpByExercise.set(l.challenge.exercise.id, challengeXpFor(l.difficulty));
  }
}
const client = await new MongoClient(process.env.MONGODB_URI ?? "mongodb://127.0.0.1:27017").connect();
const db = client.db(process.env.MONGODB_DB ?? "baremetal");

let changed = 0;
for await (const user of db.collection("users").find({}, { projection: { _id: 1, xp: 1, xpQuota: 1 } })) {
  const passed = await db.collection("progress").find({ userId: user._id, status: "passed" }, { projection: { exerciseId: 1, xpWithheld: 1, xpPending: 1, passedAt: 1 } }).toArray();
  let xp = 0;
  const ledger = [];
  for (const p of passed) {
    if (p.xpPending) {
      const decision = (user.xpQuota ?? []).find((entry: { ref?: string }) => entry.ref === p.exerciseId);
      if (!decision) throw new Error(`Pending XP for ${user._id}/${p.exerciseId} has no recorded decision. Review it before recomputing; no award is assumed.`);
      p.xpWithheld = decision.flag != null;
      await db.collection("progress").updateOne({ _id: p._id }, { $set: { xpAwarded: decision.xp, ...(decision.flag ? { xpWithheld: true, xpFlag: decision.flag } : {}) }, $unset: { xpPending: "" } });
    }
    const award = p.xpWithheld ? 0 : (xpByExercise.get(p.exerciseId) ?? 0);
    xp += award;
    await db.collection("progress").updateOne({ _id: p._id }, { $set: { xpAwarded: award } });
    if (award > 0) ledger.push({ userId: user._id, source: isChallengeId(p.exerciseId) ? "challenge" : "lesson", ref: p.exerciseId, xp: award, at: p.passedAt ?? new Date(0) });
    // Make sure every passed lesson has a place in the review schedule.
    if (!isChallengeId(p.exerciseId) && xpByExercise.has(p.exerciseId)) {
      const first = p.passedAt ?? new Date();
      await db.collection("reviews").updateOne(
        { userId: user._id, exerciseId: p.exerciseId },
        { $setOnInsert: { dueAt: new Date(first.getTime() + REVIEW_INTERVALS_DAYS[0] * 86_400_000), interval: REVIEW_INTERVALS_DAYS[0], lastAt: first, count: 0 } },
        { upsert: true },
      );
    }
  }
  await db.collection("xp_events").deleteMany({ userId: user._id });
  if (ledger.length) await db.collection("xp_events").insertMany(ledger);
  await db.collection("users").updateOne({ _id: user._id }, { $set: { xp }, $unset: { xpQuota: "", xpDecision: "" } });
  if ((user.xp ?? 0) !== xp) changed++;
}
console.log(`XP and ledger recomputed; ${changed} account(s) updated.`);
await client.close();
