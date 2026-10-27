// Recalculates every user's XP from the lessons they've passed and the current
// lesson difficulties. Run after changing a lesson's difficulty, or once to
// give XP to accounts that passed lessons before XP existed:
//   npm run recompute-xp
import { MongoClient } from "mongodb";
import { getTracks } from "../src/lib/curriculum/index";
import { xpFor } from "../src/lib/gamification";

const xpByExercise = new Map(getTracks().flatMap((t) => t.lessons.map((l) => [l.exercise.id, xpFor(l.difficulty)] as const)));
const client = await new MongoClient(process.env.MONGODB_URI ?? "mongodb://127.0.0.1:27017").connect();
const db = client.db(process.env.MONGODB_DB ?? "baremetal");

let changed = 0;
for await (const user of db.collection("users").find({}, { projection: { _id: 1, xp: 1 } })) {
  const passed = await db.collection("progress").find({ userId: user._id, status: "passed" }, { projection: { exerciseId: 1 } }).toArray();
  let xp = 0;
  for (const p of passed) {
    const award = xpByExercise.get(p.exerciseId) ?? 0;
    xp += award;
    await db.collection("progress").updateOne({ _id: p._id }, { $set: { xpAwarded: award } });
  }
  if ((user.xp ?? 0) !== xp) {
    await db.collection("users").updateOne({ _id: user._id }, { $set: { xp } });
    changed++;
  }
}
console.log(`XP recomputed; ${changed} account(s) updated.`);
await client.close();
