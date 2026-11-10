import assert from "node:assert/strict";
import { randomBytes } from "node:crypto";
import { spawnSync } from "node:child_process";
import { fileURLToPath } from "node:url";
import { MongoClient, ObjectId } from "mongodb";
import { accountingHealth } from "../../src/lib/accounting-health";

const dbName = `bm_accounting_health_${randomBytes(8).toString("hex")}`;
assert(dbName.startsWith("bm_accounting_health_"));
const client = new MongoClient(process.env.MONGODB_URI ?? "mongodb://127.0.0.1:27017", { serverSelectionTimeoutMS: 5000 });
const db = client.db(dbName);
const now = new Date("2026-11-10T12:00:00Z");
const recent = new Date(+now - 60_000);
const old = new Date(+now - 2 * 60 * 60_000);
let connected = false;
try {
  await client.connect();
  connected = true;
  assert.equal((await accountingHealth(db, now)).needsAttention, false, "empty installations are healthy");
  const cli = () => spawnSync(process.execPath, ["--import", "tsx", fileURLToPath(new URL("../accounting-health.mts", import.meta.url))], {
    env: { ...process.env, MONGODB_DB: dbName }, encoding: "utf8", timeout: 15_000,
  });
  const emptyCli = cli();
  assert.equal(emptyCli.status, 0, emptyCli.stderr);
  assert.equal(JSON.parse(emptyCli.stdout).needsAttention, false);
  const owner = new ObjectId();
  await db.collection("users").insertOne({ _id: owner, email: "private-account@example.test" });
  await db.collection("reviews").insertOne({ userId: owner, pendingEvent: { at: recent, exerciseId: "private/lesson" } });
  await db.collection("tutor_reservations").insertOne({ userId: owner.toHexString(), state: "active", createdAt: recent });
  await db.collection("progress").insertOne({ userId: owner, xpPending: true, passedAt: recent });
  await db.collection("tutor_spend").insertOne({ key: `u:${owner}:2026-11-10`, micro: 10 });
  const healthy = await accountingHealth(db, now);
  assert.equal(healthy.needsAttention, false, "live work is not reported as stalled");
  assert.equal(healthy.counts.pendingReviewEvents, 1);
  assert.equal(healthy.counts.unresolvedReservations, 1);
  assert.equal(healthy.counts.pendingXp, 1);
  await db.collection("reviews").insertMany([{ pendingEvent: { at: old } }, { pendingEvent: {} }]);
  await db.collection("tutor_reservations").insertMany([
    { state: "active", createdAt: old }, { state: "settling", createdAt: old },
    { state: "broken", createdAt: recent }, { state: "settled", createdAt: old },
  ]);
  await db.collection("progress").insertMany([{ xpPending: true, passedAt: old }, { xpPending: true }]);
  await db.collection("tutor_spend").insertMany([
    { key: `u:${new ObjectId()}:2026-11-10`, micro: 4 },
    { key: "u:malformed:2026-11-10", micro: 3 },
    { key: "g:2026-11-10", padding: "x".repeat(8 * 1024 * 1024) },
  ]);
  const report = await accountingHealth(db, now);
  assert.equal(report.needsAttention, true);
  assert.deepEqual(report.counts, { pendingReviewEvents: 3, staleReviewEvents: 2, unresolvedReservations: 4, staleReservations: 3, pendingXp: 3, staleXp: 2, orphanedPersonalBalances: 2, largeSpendDocuments: 1 });
  const json = JSON.stringify(report);
  assert(!json.includes(owner.toHexString()) && !json.includes("private") && !json.includes("micro"), "diagnostics contain aggregate counts only");
  assert.equal(await db.collection("reviews").countDocuments(), 3, "checks do not repair or delete data");
  assert.equal(await db.collection("tutor_spend").countDocuments(), 4);
  const tenMinutes = new Date(+now - 10 * 60_000);
  const oneHour = new Date(+now - 60 * 60_000);
  const reviewBoundary = await db.collection("reviews").insertOne({ pendingEvent: { at: new Date(+tenMinutes + 1) } });
  const xpBoundary = await db.collection("progress").insertOne({ xpPending: true, passedAt: new Date(+tenMinutes + 1) });
  const callBoundary = await db.collection("tutor_reservations").insertOne({ state: "active", createdAt: new Date(+oneHour + 1) });
  const beforeBoundary = await accountingHealth(db, now);
  assert.equal(beforeBoundary.counts.staleReviewEvents, report.counts.staleReviewEvents);
  assert.equal(beforeBoundary.counts.staleXp, report.counts.staleXp);
  assert.equal(beforeBoundary.counts.staleReservations, report.counts.staleReservations);
  await db.collection("reviews").updateOne({ _id: reviewBoundary.insertedId }, { $set: { "pendingEvent.at": tenMinutes } });
  await db.collection("progress").updateOne({ _id: xpBoundary.insertedId }, { $set: { passedAt: tenMinutes } });
  await db.collection("tutor_reservations").updateOne({ _id: callBoundary.insertedId }, { $set: { createdAt: oneHour } });
  const atBoundary = await accountingHealth(db, now);
  assert.equal(atBoundary.counts.staleReviewEvents, report.counts.staleReviewEvents + 1);
  assert.equal(atBoundary.counts.staleXp, report.counts.staleXp + 1);
  assert.equal(atBoundary.counts.staleReservations, report.counts.staleReservations + 1);
  const alertCli = cli();
  assert.equal(alertCli.status, 1, alertCli.stderr);
  assert.equal(JSON.parse(alertCli.stdout).needsAttention, true, "monitoring can alert on a nonzero exit");
  console.log("Read-only accounting health detection, live/stale boundaries and privacy checks passed.");
} finally {
  if (connected) await db.dropDatabase();
  await client.close();
}
