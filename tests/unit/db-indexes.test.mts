import test from "node:test";
import assert from "node:assert/strict";
import type { Db } from "mongodb";
import { ensureIndexes } from "../../src/lib/db";

function database(indexError?: Error) {
  const indexes: { collection: string; keys: unknown; options: unknown }[] = [];
  let migrated = false;
  const db = {
    collection: (collection: string) => ({
      createIndex: async (keys: Record<string, number>, options?: { unique?: boolean }) => {
        indexes.push({ collection, keys, options });
        if (collection === "tutor_spend" && keys.key && options?.unique && indexError) throw indexError;
        return "index";
      },
      dropIndex: async () => undefined,
      updateMany: async () => { migrated = true; },
    }),
  } as unknown as Db;
  return { db, indexes, migrated: () => migrated };
}

test("database initialization requires the unique tutor balance key before completing", async () => {
  const fixture = database();
  await ensureIndexes(fixture.db);
  assert(fixture.indexes.some((index) => index.collection === "tutor_spend"
    && JSON.stringify(index.keys) === '{"key":1}'
    && (index.options as { unique?: boolean })?.unique === true));
  assert.equal(fixture.migrated(), true);
});

test("duplicate balance keys fail database initialization instead of permitting uncapped reservations", async () => {
  const duplicate = Object.assign(new Error("duplicate keys"), { code: 11000 });
  const fixture = database(duplicate);
  await assert.rejects(ensureIndexes(fixture.db), (error: Error) => {
    assert.match(error.message, /Required tutor_spend unique key index/);
    assert.equal(error.cause, duplicate);
    return true;
  });
  assert.equal(fixture.migrated(), false, "initialization must not continue after a critical index failure");
});

test("index permission and availability failures also fail closed", async () => {
  const fixture = database(new Error("not authorized to create indexes"));
  await assert.rejects(ensureIndexes(fixture.db), /database access is disabled/);
  assert.equal(fixture.migrated(), false);
});
