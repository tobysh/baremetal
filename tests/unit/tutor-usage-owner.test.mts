import test from "node:test";
import assert from "node:assert/strict";

process.env.AUTH_SECRET = "unit-test-secret-unit-test-secret-0123456789";
const b = await import("../../src/lib/tutor-budget");

type Row = Record<string, unknown>;
const fakeDb = () => {
  const rows: Row[] = [];
  const match = (r: Row, f: Row) => Object.entries(f).every(([k, v]) => r[k] === v);
  return {
    rows,
    collection: () => ({
      insertOne: async (d: Row) => void rows.push(d),
      deleteMany: async (f: Row) => { for (let i = rows.length - 1; i >= 0; i--) if (match(rows[i], f)) rows.splice(i, 1); },
      find: (f: Row, o?: { projection?: Record<string, number> }) => ({
        toArray: async () => rows.filter((r) => match(r, f)).map((r) => Object.fromEntries(Object.entries(r).filter(([k]) => !o?.projection || o.projection[k] !== 0))),
      }),
    }),
  };
};

test("usage rows carry a keyed owner, never the raw user id", async () => {
  const db = fakeDb();
  const id = "64b7f0f0f0f0f0f0f0f0f0f0";
  await b.recordTutorUsage(db as never, id, { exerciseId: "c/x", responseCacheHit: false });
  assert.equal(db.rows.length, 1);
  assert.ok(!JSON.stringify(db.rows[0]).includes(id));
  assert.equal(db.rows[0].owner, b.tutorUsageOwner(id));
  assert.notEqual(b.tutorUsageOwner(id), b.tutorUsageOwner("64b7f0f0f0f0f0f0f0f0f0f1"));
});

test("deleting and exporting by owner touch only that user's rows (legacy rows untouched)", async () => {
  const db = fakeDb();
  await b.recordTutorUsage(db as never, "a", { exerciseId: "e1", responseCacheHit: true });
  await b.recordTutorUsage(db as never, "b", { exerciseId: "e2", responseCacheHit: false });
  db.rows.push({ exerciseId: "old", ts: new Date() });
  const exported = await b.exportTutorUsage(db as never, "a");
  assert.deepEqual(exported.map((r) => r.exerciseId), ["e1"]);
  assert.ok(exported.every((r) => !("owner" in r)));
  await b.deleteTutorUsage(db as never, "a");
  assert.deepEqual(db.rows.map((r) => r.exerciseId), ["e2", "old"]);
});
