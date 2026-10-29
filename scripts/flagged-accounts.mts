// Lists accounts with withheld XP (passes that were too fast, too many, or over the daily cap) so a person can
// review them, and clears the flags on an account you decide is honest:
//   npm run flagged-accounts                    list, most flags first
//   npm run flagged-accounts -- --clear <email> unflag that account's passes; then run recompute-xp
// Run recompute-xp with the site stopped.
import { MongoClient } from "mongodb";

const client = await new MongoClient(process.env.MONGODB_URI ?? "mongodb://127.0.0.1:27017").connect();
const db = client.db(process.env.MONGODB_DB ?? "baremetal");
const clear = process.argv.indexOf("--clear");

if (clear > 0) {
  const email = process.argv[clear + 1]?.trim().toLowerCase();
  const user = email ? await db.collection("users").findOne({ email }, { projection: { _id: 1 } }) : null;
  if (!user) console.error(`No account found for ${email ?? "(no email given)"}.`);
  else {
    const res = await db.collection("progress").updateMany({ userId: user._id, xpWithheld: true }, { $unset: { xpWithheld: "", xpFlag: "" } });
    console.log(`Cleared ${res.modifiedCount} flagged pass(es) for ${email}. Stop the site and run: npm run recompute-xp`);
  }
} else {
  const rows = await db
    .collection("progress")
    .aggregate([
      { $match: { $or: [{ xpWithheld: true }, { xpPending: true }] } },
      { $group: { _id: "$userId", n: { $sum: 1 }, reasons: { $push: { $cond: ["$xpPending", "pending-reconciliation", { $ifNull: ["$xpFlag", "pace"] }] } }, last: { $max: "$passedAt" } } },
      { $sort: { n: -1 } },
      { $limit: 100 },
      { $lookup: { from: "users", localField: "_id", foreignField: "_id", as: "u" } },
    ])
    .toArray();
  if (!rows.length) console.log("No flagged accounts.");
  for (const r of rows) {
    const counts: Record<string, number> = {};
    for (const x of r.reasons as string[]) counts[x] = (counts[x] ?? 0) + 1;
    console.log(`${String(r.n).padStart(4)}  ${r.u[0]?.email ?? r._id}  ${JSON.stringify(counts)}  last ${r.last?.toISOString?.() ?? "?"}`);
  }
}
await client.close();
