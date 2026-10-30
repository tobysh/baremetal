// Reports (and with --apply deletes) rows left behind by account deletion: documents whose userId has no user.
//   npx tsx --env-file-if-exists=.env.local scripts/sweep-orphans.mts          dry run
//   npx tsx --env-file-if-exists=.env.local scripts/sweep-orphans.mts --apply  delete them
import { MongoClient } from "mongodb";

const apply = process.argv.includes("--apply");
const COLLECTIONS = ["progress", "tutor_threads", "xp_events", "achievements", "reviews"];
const client = await new MongoClient(process.env.MONGODB_URI ?? "mongodb://127.0.0.1:27017").connect();
const db = client.db(process.env.MONGODB_DB ?? "baremetal");

const live = new Set((await db.collection("users").find({}, { projection: { _id: 1 } }).toArray()).map((u) => u._id.toHexString()));
let total = 0;
for (const name of COLLECTIONS) {
  const col = db.collection(name);
  const orphans = [];
  for (const id of await col.distinct("userId")) if (!live.has(String(id))) orphans.push(id);
  const count = orphans.length ? await col.countDocuments({ userId: { $in: orphans } }) : 0;
  total += count;
  console.log(`${name.padEnd(14)} ${count} orphaned row(s) from ${orphans.length} missing user(s)`);
  if (apply && orphans.length) await col.deleteMany({ userId: { $in: orphans } });
}
console.log(apply ? `Deleted ${total} row(s).` : `${total} row(s) would be deleted. Re-run with --apply.`);
await client.close();
