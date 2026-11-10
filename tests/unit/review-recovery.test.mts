import test from "node:test";
import assert from "node:assert/strict";
import { ObjectId, type Db } from "mongodb";
import type { ReviewDoc, ReviewEventDoc } from "../../src/lib/models";
import { settleReviewCard, skipReview } from "../../src/lib/review-cards";
import { recordReviewPass } from "../../src/lib/progress";
import { flushPendingReviewEvent, reconcilePendingReviewEvents, reviewEvent, updateReviewWithEvent } from "../../src/lib/review-events";

type Row = Record<string, unknown>;
type Fault = "schedule-before" | "schedule-after" | "event-before" | "event-after" | "ack-before" | "ack-after";
const same = (a: unknown, b: unknown) => a instanceof ObjectId && b instanceof ObjectId ? a.equals(b) : a instanceof Date && b instanceof Date ? +a === +b : a === b;
const field = (row: Row, key: string) => key.split(".").reduce<unknown>((value, part) => (value as Row | undefined)?.[part], row);
const matches = (row: Row, filter: Row) => Object.entries(filter).every(([key, value]) => {
  const actual = field(row, key);
  if (value && typeof value === "object" && "$exists" in value) return (actual !== undefined) === value.$exists;
  return same(actual, value);
});

function database() {
  const uid = new ObjectId();
  const review: ReviewDoc = { _id: new ObjectId(), userId: uid, exerciseId: "test/review", dueAt: new Date(Date.now() - 100_000), lastAt: new Date(Date.now() - 7 * 86_400_000), interval: 7, count: 0 };
  const events: ReviewEventDoc[] = [];
  let fault: Fault | undefined;
  let ownerExists = true;
  let deleteOwnerAfterInsert = false;
  const trip = (point: Fault) => { if (fault === point) { fault = undefined; throw new Error(`injected ${point}`); } };
  const db = { collection: (name: string) => ({
    findOne: async (filter: Row) => {
      if (name === "users") return ownerExists ? { _id: uid } : null;
      const row = (name === "reviews" ? [review] : events).find((row) => matches(row, filter));
      return row ? { ...row } : null;
    },
    find: (filter: Row) => ({ async *[Symbol.asyncIterator]() { if (matches(review, filter)) yield { ...review }; } }),
    updateOne: async (filter: Row, update: { $set?: Row; $setOnInsert?: Row; $unset?: Row; $inc?: Record<string, number> }) => {
      if (name === "review_events") {
        trip("event-before");
        if (!events.some((event) => matches(event, filter))) events.push({ ...update.$setOnInsert } as ReviewEventDoc);
        if (deleteOwnerAfterInsert) ownerExists = false;
        trip("event-after");
        return { modifiedCount: 0 };
      }
      const ack = update.$unset !== undefined && "pendingEvent" in update.$unset;
      trip(ack ? "ack-before" : "schedule-before");
      if (!matches(review, filter)) return { modifiedCount: 0 };
      Object.assign(review, update.$set);
      for (const [key, value] of Object.entries(update.$inc ?? {})) (review as unknown as Row)[key] = Number((review as unknown as Row)[key]) + value;
      for (const key of Object.keys(update.$unset ?? {})) delete (review as unknown as Row)[key];
      trip(ack ? "ack-after" : "schedule-after");
      return { modifiedCount: 1 };
    },
    deleteOne: async (filter: Row) => {
      const index = events.findIndex((event) => matches(event, filter));
      if (index >= 0) events.splice(index, 1);
    },
  }) } as unknown as Db;
  const cache = globalThis as unknown as { __mongo?: { db: Promise<Db> } };
  cache.__mongo = { db: Promise.resolve(db) };
  return { uid: uid.toHexString(), review, events, db, fail: (at: Fault) => { fault = at; }, deleteOwner: () => { ownerExists = false; }, deleteOwnerAfterInsert: () => { deleteOwnerAfterInsert = true; } };
}

for (const source of ["card", "coding", "skip"] as const) {
  test(`${source}: failed history insertion survives retry without moving schedule twice`, async () => {
    const f = database();
    const submit = () => source === "card" ? settleReviewCard(f.uid, f.review.exerciseId, { correct: false, questionIndex: 2 }) : source === "coding" ? recordReviewPass(f.uid, f.review.exerciseId) : skipReview(f.uid, f.review.exerciseId, "UTC");
    f.fail("event-before");
    await assert.rejects(submit(), /injected event-before/);
    const due = +f.review.dueAt;
    assert(f.review.pendingEvent);
    const pendingId = f.review.pendingEvent._id;
    assert.equal(await submit(), source === "card" ? null : false);
    assert.equal(+f.review.dueAt, due);
    assert.equal(f.events.length, 1);
    assert(f.events[0]._id.equals(pendingId));
    assert.equal(f.events[0].source, source);
    assert.equal(f.review.pendingEvent, undefined);
    assert.equal(f.review.count, source === "coding" ? 1 : 0);
  });
}

for (const fault of ["schedule-after", "event-after", "ack-before", "ack-after"] as const) {
  test(`startup recovers ambiguous ${fault} without duplicate history`, async () => {
    const f = database();
    f.fail(fault);
    await assert.rejects(settleReviewCard(f.uid, f.review.exerciseId, { correct: true, questionIndex: 0 }), /injected/);
    const due = +f.review.dueAt;
    await reconcilePendingReviewEvents();
    await reconcilePendingReviewEvents();
    assert.equal(f.events.length, 1);
    assert.equal(f.review.pendingEvent, undefined);
    assert.equal(+f.review.dueAt, due);
  });
}

test("failed atomic schedule write creates neither event nor outbox", async () => {
  const f = database();
  const due = +f.review.dueAt;
  f.fail("schedule-before");
  await assert.rejects(recordReviewPass(f.uid, f.review.exerciseId), /injected/);
  assert.equal(f.events.length, 0);
  assert.equal(f.review.pendingEvent, undefined);
  assert.equal(+f.review.dueAt, due);
});

test("recovery reports persistence failures and retains intent for the next sweep", async () => {
  const f = database();
  f.fail("event-before");
  await assert.rejects(recordReviewPass(f.uid, f.review.exerciseId), /injected/);
  f.fail("event-before");
  const originalError = console.error;
  const errors: unknown[][] = [];
  try {
    console.error = (...args: unknown[]) => { errors.push(args); };
    assert.deepEqual(await reconcilePendingReviewEvents(), { recovered: 0, failed: 1 });
  } finally {
    console.error = originalError;
  }
  assert.equal(errors.length, 1);
  assert(f.review.pendingEvent);
  assert.deepEqual(await reconcilePendingReviewEvents(), { recovered: 1, failed: 0 });
  assert.equal(f.events.length, 1);
});

test("concurrent submissions advance the schedule and coding counter once", async () => {
  const f = database();
  const results = await Promise.all(Array.from({ length: 8 }, () => recordReviewPass(f.uid, f.review.exerciseId)));
  assert.equal(results.filter(Boolean).length, 1);
  assert.equal(f.review.count, 1);
  assert.equal(f.events.length, 1);
});

test("stale schedule snapshots cannot overwrite a concurrent reset or pending event", async () => {
  const f = database();
  const before = { ...f.review };
  const event = reviewEvent({ userId: before.userId, exerciseId: before.exerciseId, source: "coding", before, intervalAfter: 21, now: new Date(), correct: true });
  f.review.lastAt = new Date(Date.now() + 1000);
  assert.equal(await updateReviewWithEvent(f.db, before, event, { $set: { interval: 21 } }), false);
  assert.equal(f.events.length, 0);
  f.review.lastAt = before.lastAt;
  f.review.pendingEvent = event;
  assert.equal(await updateReviewWithEvent(f.db, before, event, { $set: { interval: 21 } }), false);
  assert.equal(f.events.length, 0);
});

test("replaying an older acknowledgement cannot clear a newer queued event", async () => {
  const f = database();
  const input = { userId: f.review.userId, exerciseId: f.review.exerciseId, source: "coding" as const, before: f.review, intervalAfter: 21, now: new Date(), correct: true };
  const old = reviewEvent(input), newer = reviewEvent(input);
  const stale = { ...f.review, pendingEvent: old };
  f.review.pendingEvent = newer;
  await flushPendingReviewEvent(f.db, stale);
  assert(f.review.pendingEvent._id.equals(newer._id));
  await flushPendingReviewEvent(f.db, f.review);
  assert.equal(f.review.pendingEvent, undefined);
  assert.equal(f.events.length, 2);
});

for (const timing of ["before", "during"] as const) {
  test(`recovery does not resurrect history when account deletion happens ${timing} insertion`, async () => {
    const f = database();
    f.fail("event-before");
    await assert.rejects(recordReviewPass(f.uid, f.review.exerciseId), /injected/);
    if (timing === "before") f.deleteOwner();
    else f.deleteOwnerAfterInsert();
    await reconcilePendingReviewEvents();
    assert.equal(f.events.length, 0);
    assert.equal(f.review.pendingEvent, undefined);
  });
}
