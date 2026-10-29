import assert from "node:assert/strict";
import { MongoClient } from "mongodb";
import { reserveBudget } from "../../src/lib/tutor-budget";

const client = new MongoClient(process.env.MONGODB_URI ?? "mongodb://127.0.0.1:27017", { serverSelectionTimeoutMS: 5000 });
const db = client.db(`bm_budget_check_${process.pid}_${Date.now()}`);
process.env.TUTOR_BUDGET_USER_DAY_USD = "0.000004";
process.env.TUTOR_BUDGET_GLOBAL_DAY_USD = "0.000006";
try {
  await client.connect();
  await db.collection("tutor_spend").createIndex({key: 1}, {unique: true});
  const holds = await Promise.all(Array.from({length: 10}, () => reserveBudget(db, "one", 1)));
  assert.equal(holds.filter(Boolean).length, 4, "concurrent reservations respect the user ceiling");
  const second = await Promise.all(Array.from({length: 10}, () => reserveBudget(db, "two", 1)));
  assert.equal(second.filter(Boolean).length, 2, "global ceiling covers multiple users");
  await Promise.all(holds.filter((h) => h !== null).map((h) => h(null)));
  const refunded = await reserveBudget(db, "one", 4);
  assert(refunded, "refund frees capacity");
  await refunded({inputTokens: 1, outputTokens: 0, cacheReadTokens: 0, cacheWriteTokens: 0});
  await refunded(null); // idempotent settlement
  const user = await db.collection("tutor_spend").findOne({key: /^u:one:/});
  assert.equal(user?.micro, 1, "settlement charges actual usage once");
  assert.equal(await reserveBudget(db, "one", 5), null, "a reservation larger than the ceiling is denied");
  console.log("Tutor concurrent user/global reservation, refund and settlement checks passed.");
} finally {
  await db.dropDatabase();
  await client.close();
}
