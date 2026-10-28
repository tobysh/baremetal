// Awards any achievements learners have already earned (run after adding one to src/lib/achievements.ts):
//   npm run backfill-achievements
import { MongoClient } from "mongodb";
import { ACHIEVEMENTS } from "../src/lib/achievements";
import { getTracks } from "../src/lib/curriculum/index";
import { buildContext } from "../src/lib/rewards-context";

const client = await new MongoClient(process.env.MONGODB_URI ?? "mongodb://127.0.0.1:27017").connect();
const db = client.db(process.env.MONGODB_DB ?? "baremetal");
const tracks = getTracks();
let added = 0;
for await (const user of db.collection("users").find({}, { projection: { xp: 1, timezone: 1, weeklyGoal: 1 } })) {
  const [progress, reviews, have] = await Promise.all([
    db.collection("progress").find({ userId: user._id }, { projection: { code: 0 } }).toArray(),
    db.collection("reviews").find({ userId: user._id }).toArray(),
    db.collection("achievements").find({ userId: user._id }).toArray(),
  ]);
  const owned = new Set(have.map((a) => a.id));
  const ctx = buildContext(tracks, progress as never, reviews as never, user as never);
  for (const a of ACHIEVEMENTS) {
    if (owned.has(a.id) || !a.check(ctx)) continue;
    await db.collection("achievements").updateOne({ userId: user._id, id: a.id }, { $setOnInsert: { unlockedAt: new Date() } }, { upsert: true });
    added++;
  }
}
console.log(`${added} achievement(s) awarded.`);
await client.close();
