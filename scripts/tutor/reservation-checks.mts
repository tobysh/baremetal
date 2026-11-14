import assert from "node:assert/strict";
import { MongoClient, ObjectId, type Db } from "mongodb";
import { reconcileTutorReservations, reserveBudget } from "../../src/lib/tutor-budget";

const client = new MongoClient(process.env.MONGODB_URI ?? "mongodb://127.0.0.1:27017", { serverSelectionTimeoutMS: 5000 });
const db = client.db(`bm_budget_check_${process.pid}_${Date.now()}`);
const users = new Map<string, ObjectId>();
const idOf = (name: string) => { if (!users.has(name)) users.set(name, new ObjectId()); return users.get(name)!.toHexString(); };
process.env.TUTOR_BUDGET_USER_DAY_USD = "0.000004";
process.env.TUTOR_BUDGET_GLOBAL_DAY_USD = "0.000006";
try {
  await client.connect();
  for (const name of ["one", "two", "partial", "ambiguous", "hold", "stale", "unstarted", "concurrent", "deleted", "deleted-hold", "cleanup", "broken", "later"]) await db.collection("users").insertOne({ _id: new ObjectId(idOf(name)) });
  await db.collection("tutor_spend").createIndex({key: 1}, {unique: true});
  const holds = await Promise.all(Array.from({length: 10}, () => reserveBudget(db, idOf("one"), 1)));
  assert.equal(holds.filter(Boolean).length, 4, "concurrent reservations respect the user ceiling");
  const second = await Promise.all(Array.from({length: 10}, () => reserveBudget(db, idOf("two"), 1)));
  assert.equal(second.filter(Boolean).length, 2, "global ceiling covers multiple users");
  await Promise.all(holds.filter((h) => h !== null).map((h) => h(null)));
  const refunded = await reserveBudget(db, idOf("one"), 4);
  assert(refunded, "refund frees capacity");
  await refunded({inputTokens: 1, outputTokens: 0, cacheReadTokens: 0, cacheWriteTokens: 0});
  await refunded(null); // idempotent settlement
  const user = await db.collection("tutor_spend").findOne({key: new RegExp(`^u:${idOf("one")}:`)});
  assert.equal(user?.micro, 1, "settlement charges actual usage once");
  assert.equal(await reserveBudget(db, idOf("one"), 5), null, "a reservation larger than the ceiling is denied");
  // Every injected write runs only against this script's unique scratch DB.
  process.env.TUTOR_BUDGET_USER_DAY_USD = "1";
  process.env.TUTOR_BUDGET_GLOBAL_DAY_USD = "10";
  const usage = { inputTokens: 2, outputTokens: 0, cacheReadTokens: 0, cacheWriteTokens: 0 };
  type WriteHook = (name: string, args: unknown[], write: () => Promise<unknown>) => Promise<unknown>;
  const injecting = (hook: WriteHook, operations = ["updateOne"]): Db => ({ collection(name: string) {
    const collection = db.collection(name);
    return new Proxy(collection, { get(target, property) {
      if (typeof property === "string" && operations.includes(property)) return (...args: unknown[]) => hook(name, args, () => (Reflect.get(target, property) as (...a: unknown[]) => Promise<unknown>).apply(target, args));
      const value = Reflect.get(target, property);
      return typeof value === "function" ? value.bind(target) : value;
    } });
  } }) as Db;
  const totals = async (user: string) => {
    const row = await db.collection("tutor_spend").findOne({ key: new RegExp(`^u:${idOf(user)}:`) });
    return row?.micro;
  };
  const reset = async () => { await db.collection("tutor_spend").deleteMany({}); await db.collection("tutor_reservations").deleteMany({}); };

  await reset();
  let failGlobal = true;
  const partialDb = injecting(async (name, args, write) => {
    if (name === "tutor_spend" && Array.isArray(args[1]) && String((args[0] as {key: string}).key).startsWith("g:") && failGlobal) {
      failGlobal = false;
      throw new Error("injected global settlement failure");
    }
    return write();
  });
  const partial = await reserveBudget(partialDb, idOf("partial"), 10);
  assert(partial);
  await assert.rejects(partial(usage), /injected/);
  assert.equal(await totals("partial"), 2, "first scope settled before failure");
  await partial(usage);
  assert.equal(await totals("partial"), 2, "retry does not apply first scope adjustment twice");
  assert.equal((await db.collection("tutor_spend").findOne({key: /^g:/}))?.micro, 2);

  await reset();
  let loseAck = true;
  const ambiguousDb = injecting(async (name, args, write) => {
    const result = await write();
    if (name === "tutor_spend" && Array.isArray(args[1]) && loseAck) {
      loseAck = false;
      throw new Error("injected lost acknowledgement");
    }
    return result;
  });
  const ambiguous = await reserveBudget(ambiguousDb, idOf("ambiguous"), 10);
  assert(ambiguous);
  await assert.rejects(ambiguous(usage), /lost acknowledgement/);
  await reconcileTutorReservations(db);
  assert.equal(await totals("ambiguous"), 2, "restart recovery does not duplicate acknowledged-or-not adjustment");
  assert.equal((await db.collection("tutor_spend").findOne({key: /^g:/}))?.micro, 2);
  await ambiguous(null);
  assert.equal(await totals("ambiguous"), 2, "durable first settlement decision wins over later refund");

  await reset();
  let holdAck = true;
  const holdDb = injecting(async (name, args, write) => {
    const result = await write();
    if (name === "tutor_spend" && !Array.isArray(args[1]) && holdAck) {
      holdAck = false;
      throw new Error("injected hold acknowledgement loss");
    }
    return result;
  });
  await assert.rejects(reserveBudget(holdDb, idOf("hold"), 10), /hold acknowledgement/);
  assert.equal(await totals("hold"), 0, "unknown hold outcome is refunded once before upstream starts");
  await reconcileTutorReservations(db);
  assert.equal(await totals("hold"), 0);

  await reset();
  const staleAt = Date.now() - 2 * 60 * 60 * 1000;
  const stale = await reserveBudget(db, idOf("stale"), 10, staleAt);
  assert(stale);
  const recovered = await reconcileTutorReservations(db);
  assert.equal(recovered.chargedConservatively, 1);
  assert.equal(await totals("stale"), 10, "unknown upstream charge retains the maximum hold");
  assert.equal((await db.collection("tutor_reservations").findOne({ userId: idOf("stale") }))?.recovery, "charge-unknown");
  await stale(null);
  assert.equal(await totals("stale"), 10, "late client settlement cannot undo a recovery decision");

  await reset();
  const brokenActivation = injecting(async (name, args, write) => {
    if (name === "tutor_reservations") throw new Error("injected journal write outage");
    return write();
  });
  await assert.rejects(reserveBudget(brokenActivation, idOf("unstarted"), 10, staleAt), /journal write outage/);
  assert.equal(await totals("unstarted"), 10, "outage leaves a durable unstarted hold");
  await reconcileTutorReservations(db);
  assert.equal(await totals("unstarted"), 0, "restart refunds abandoned pre-upstream hold");

  await reset();
  const concurrent = await reserveBudget(db, idOf("concurrent"), 10);
  assert(concurrent);
  await Promise.all([concurrent(usage), concurrent(null), concurrent(usage)]);
  const decision = await db.collection("tutor_reservations").findOne({ userId: idOf("concurrent") });
  assert.equal(await totals("concurrent"), decision?.targetMicro, "concurrent settlement uses one durable target");
  assert.equal((await db.collection("tutor_spend").findOne({key: /^g:/}))?.micro, decision?.targetMicro);
  assert(decision?.expiresAt instanceof Date, "only terminal journals receive retention expiry");
  await assert.rejects(reserveBudget(db, idOf("invalid"), NaN), /Invalid/);
  await assert.rejects(reserveBudget(db, idOf("invalid"), -1), /Invalid/);
  await reset();
  let deletedDuringWrite = false;
  const deletedId = idOf("deleted");
  const deletingDb = injecting(async (name, args, write) => {
    const result = await write();
    if (name === "tutor_spend" && Array.isArray(args[1]) && !deletedDuringWrite) {
      deletedDuringWrite = true;
      await db.collection("users").deleteOne({ _id: new ObjectId(deletedId) });
      await db.collection("tutor_spend").deleteMany({ key: new RegExp(`^u:${deletedId}:`) });
      await db.collection("tutor_reservations").deleteMany({ userId: deletedId });
    }
    return result;
  });
  const deleting = await reserveBudget(deletingDb, deletedId, 10);
  assert(deleting);
  await deleting(usage);
  assert.equal(await db.collection("tutor_spend").countDocuments({ key: new RegExp(`^u:${deletedId}:`) }), 0);
  assert.equal(await db.collection("tutor_reservations").countDocuments({ userId: deletedId }), 0);
  assert.equal((await db.collection("tutor_spend").findOne({key: /^g:/}))?.micro, 2, "account deletion preserves anonymous site spend");

  await reset();
  const deletedHoldId = idOf("deleted-hold");
  let deleteBeforeHold = true;
  const deletedHoldDb = injecting(async (name, args, write) => {
    if (name === "tutor_spend" && !Array.isArray(args[1]) && deleteBeforeHold) {
      deleteBeforeHold = false;
      await db.collection("users").deleteOne({ _id: new ObjectId(deletedHoldId) });
      await db.collection("tutor_spend").deleteMany({ key: new RegExp(`^u:${deletedHoldId}:`) });
      await db.collection("tutor_reservations").deleteMany({ userId: deletedHoldId });
    }
    return write();
  });
  await assert.rejects(reserveBudget(deletedHoldDb, deletedHoldId, 10), /expired before activation/);
  assert.equal(await db.collection("tutor_spend").countDocuments({ key: new RegExp(`^u:${deletedHoldId}:`) }), 0, "late acquisition write cannot resurrect deleted personal spend");
  assert.equal(await db.collection("tutor_reservations").countDocuments({ userId: deletedHoldId }), 0);
  assert.equal((await db.collection("tutor_spend").findOne({key: /^g:/}))?.micro, 10, "missing deletion journal leaves the anonymous hold conservative");

  await reset();
  const cleanup = await reserveBudget(db, idOf("cleanup"), 10);
  assert(cleanup);
  await db.collection("users").deleteOne({ _id: new ObjectId(idOf("cleanup")) });
  let failCleanup = true;
  const cleanupDb = injecting(async (name, args, write) => {
    if (name === "tutor_spend" && failCleanup) {
      failCleanup = false;
      throw new Error("injected deleted-owner cleanup failure");
    }
    return write();
  }, ["deleteOne"]);
  // Simulate a worker dying after its durable decision and before owner cleanup.
  await db.collection("tutor_reservations").updateOne({ userId: idOf("cleanup") }, { $set: { state: "settling", targetMicro: 2 } });
  const cleanupReport = await reconcileTutorReservations(cleanupDb);
  assert.equal(cleanupReport.failed, 1);
  assert.equal((await db.collection("tutor_reservations").findOne({ userId: idOf("cleanup") }))?.state, "settling", "failed cleanup remains discoverable");
  await reconcileTutorReservations(db);
  assert.equal(await db.collection("tutor_reservations").countDocuments({ userId: idOf("cleanup") }), 0);
  assert.equal(await totals("cleanup"), undefined, "later sweep removes resurrected personal totals");
  assert.equal((await db.collection("tutor_spend").findOne({key: /^g:/}))?.micro, 2);

  const prepareBlockedBatch = async () => {
    await reset();
    assert(await reserveBudget(db, idOf("broken"), 10, Date.now() - 1000));
    assert(await reserveBudget(db, idOf("later"), 10));
    await db.collection("tutor_reservations").updateMany({}, { $set: { state: "settling", targetMicro: 2 } });
    return injecting(async (name, args, write) => {
      if (name === "tutor_spend" && String((args[0] as { key?: string }).key).startsWith(`u:${idOf("broken")}:`)) throw new Error("injected persistent single-row failure");
      return write();
    });
  };
  const blockedDb = await prepareBlockedBatch();
  const batch = await reconcileTutorReservations(blockedDb);
  assert.equal(batch.failed, 1);
  assert.equal(batch.settled, 1, "one failed journal does not abort later recovery");
  assert.equal(await totals("later"), 2);
  const boundedDb = await prepareBlockedBatch();
  assert.equal((await reconcileTutorReservations(boundedDb, Date.now(), 1)).failed, 1);
  assert.equal((await reconcileTutorReservations(boundedDb, Date.now() + 1, 1)).settled, 1, "failed oldest journal rotates behind unattempted work in bounded batches");
  assert.equal(await totals("later"), 2);
  await reconcileTutorReservations(db);
  assert.equal(await totals("broken"), 2, "retained failed journal recovers once its storage error clears");
  console.log("Tutor concurrent caps, refund, partial/ambiguous writes, restart recovery, cleanup failures and bounded recovery checks passed.");
} finally {
  await db.dropDatabase();
  await client.close();
}
