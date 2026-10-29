import { MongoClient, type Db } from "mongodb";

const uri = process.env.MONGODB_URI ?? "mongodb://127.0.0.1:27017";
const dbName = process.env.MONGODB_DB ?? "baremetal";

type Cache = { client: MongoClient; db: Promise<Db> };
const globalForMongo = globalThis as unknown as { __mongo?: Cache };

async function connect(client: MongoClient): Promise<Db> {
  await client.connect();
  const db = client.db(dbName);
  await ensureIndexes(db);
  return db;
}

async function ensureIndexes(db: Db) {
  await Promise.all([
    db.collection("users").createIndex({ email: 1 }, { unique: true }),
    db.collection("users").createIndex({ username: 1 }, { unique: true, partialFilterExpression: { username: { $type: "string" } } }),
    db.collection("users").createIndex({ profilePublic: 1, xp: -1 }),
    db.collection("sessions").createIndex({ tokenHash: 1 }, { unique: true }),
    db.collection("sessions").createIndex({ expiresAt: 1 }, { expireAfterSeconds: 0 }),
    db.collection("sessions").createIndex({ userId: 1 }),
    db.collection("progress").createIndex({ userId: 1, exerciseId: 1 }, { unique: true }),
    db.collection("xp_events").createIndex({ userId: 1, source: 1, ref: 1 }, { unique: true }),
    db.collection("xp_events").createIndex({ at: 1, userId: 1 }),
    db.collection("achievements").createIndex({ userId: 1, id: 1 }, { unique: true }),
    db.collection("reviews").createIndex({ userId: 1, exerciseId: 1 }, { unique: true }),
    db.collection("reviews").createIndex({ userId: 1, dueAt: 1 }),
    db.collection("tutor_threads").createIndex({ userId: 1, lessonId: 1 }, { unique: true }),
    db.collection("rate_limits").createIndex({ key: 1 }, { unique: true }),
    db.collection("rate_limits").createIndex({ expiresAt: 1 }, { expireAfterSeconds: 0 }),
    // Analytics events are kept for 180 days, as stated in the privacy notice.
    db.collection("events").createIndex({ ts: 1 }, { expireAfterSeconds: 60 * 60 * 24 * 180 }),
    db.collection("events").createIndex({ type: 1, ts: -1 }),
    // Security log: 90 days, as stated in the privacy notice.
    db.collection("auth_events").createIndex({ ts: 1 }, { expireAfterSeconds: 60 * 60 * 24 * 90 }),
    db.collection("auth_events").createIndex({ type: 1, ts: -1 }),
    db.collection("login_failures").createIndex({ key: 1 }, { unique: true }),
    db.collection("login_failures").createIndex({ expiresAt: 1 }, { expireAfterSeconds: 0 }),
    db.collection("email_queue").createIndex({ nextAttempt: 1, createdAt: 1 }),
    db.collection("email_queue").createIndex({ expiresAt: 1 }, { expireAfterSeconds: 0 }),
    db.collection("email_verifications").createIndex({ tokenHash: 1 }, { unique: true }),
    db.collection("email_verifications").createIndex({ expiresAt: 1 }, { expireAfterSeconds: 0 }),
    db.collection("password_resets").createIndex({ tokenHash: 1 }, { unique: true }),
    db.collection("password_resets").createIndex({ expiresAt: 1 }, { expireAfterSeconds: 0 }),
    db.collection("pending_2fa").createIndex({ tokenHash: 1 }, { unique: true }),
    db.collection("pending_2fa").createIndex({ expiresAt: 1 }, { expireAfterSeconds: 0 }),
    // Tutor response cache: each entry carries its own expireAt (30 days, 90 once it is popular), so answers refresh as lessons and models change.
    db.collection("tutor_cache").createIndex({ key: 1 }, { unique: true }),
    db.collection("tutor_cache").createIndex({ expireAt: 1 }, { expireAfterSeconds: 0 }),
    db.collection("tutor_cache").createIndex({ popular: 1, hits: -1 }),
    db.collection("tutor_cache").createIndex({ exerciseId: 1, hits: -1 }),
    // One document per user/site and UTC day. Kept 60 days; only the current day is read.
    db.collection("tutor_spend").createIndex({ key: 1 }, { unique: true }).catch((e) => console.warn("[db] tutor_spend unique index not created (duplicate keys?):", e?.message)),
    db.collection("tutor_spend").createIndex({ createdAt: 1 }, { expireAfterSeconds: 60 * 60 * 24 * 60 }),
    db.collection("tutor_usage").createIndex({ ts: 1 }, { expireAfterSeconds: 60 * 60 * 24 * 180 }),
  ]);
  await migrateTutorCache(db);
}

/** Older cache entries were expired by a createdAt TTL index; they now carry expireAt like new ones. */
async function migrateTutorCache(db: Db) {
  const cache = db.collection("tutor_cache");
  await cache.dropIndex("createdAt_1").catch(() => undefined); // already gone, or never existed
  await cache
    .updateMany({ expireAt: { $exists: false } }, [{ $set: { expireAt: { $dateAdd: { startDate: { $ifNull: ["$createdAt", "$$NOW"] }, unit: "day", amount: 30 } } } }])
    .catch((e) => console.warn("[db] tutor_cache migration skipped:", e?.message));
}

export function getDb(): Promise<Db> {
  if (!globalForMongo.__mongo) {
    const client = new MongoClient(uri, { serverSelectionTimeoutMS: 5000 });
    const db = connect(client);
    globalForMongo.__mongo = { client, db };
    // Let the next request retry if the first connection attempt fails.
    db.catch(() => {
      globalForMongo.__mongo = undefined;
    });
  }
  return globalForMongo.__mongo.db;
}
