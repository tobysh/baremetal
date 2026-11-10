// Exact canonical-EJSON roundtrip against explicitly configured loopback MongoDB only.
import assert from "node:assert/strict";
import { execFile } from "node:child_process";
import { promisify } from "node:util";
import { cp, lstat, mkdtemp, readFile, rm, symlink, unlink, writeFile } from "node:fs/promises";
import { tmpdir } from "node:os";
import path from "node:path";
import { createHash, randomBytes } from "node:crypto";
import { gunzipSync, gzipSync } from "node:zlib";
import { BSON, MongoClient, ObjectId, type Db, type Document } from "mongodb";
import { dumpDb, loadInto, readManifest, validateBackup, verifyBackup, type Manifest } from "../lib/backup.mjs";

const configuredUri = process.env.MONGODB_URI;
if (!configuredUri) throw new Error("Backup regression requires an explicit loopback MONGODB_URI; no default database is used");
let configured: URL;
try { configured = new URL(configuredUri); }
catch { throw new Error("Backup regression requires a valid loopback MONGODB_URI"); }
if (configured.protocol !== "mongodb:" || !["localhost", "127.0.0.1", "[::1]"].includes(configured.hostname.toLowerCase()) || [...configured.searchParams.keys()].some((key) => key.toLowerCase().startsWith("proxy"))) {
  throw new Error("Backup regression refuses non-loopback, multi-host, SRV or proxied MongoDB URIs");
}
// Do not discover other replica-set members beyond the explicitly selected local seed.
configured.searchParams.set("directConnection", "true");
const testUri = configured.toString();
const prefix = `bm_backup_check_${randomBytes(6).toString("hex")}`;
const client = new MongoClient(testUri, { serverSelectionTimeoutMS: 5000 });
const databases: Db[] = [];
const database = (suffix: string) => {
  const name = `${prefix}_${suffix}`;
  assert(/^bm_backup_check_[a-f0-9]+_[a-z]+$/.test(name));
  const db = client.db(name);
  databases.push(db);
  return db;
};
const scratch = await mkdtemp(path.join(tmpdir(), "bm-backup-check-"));
const oldBackupDir = process.env.BACKUP_DIR;
process.env.BACKUP_DIR = path.join(scratch, "backups");
const { EJSON, Double, Int32, Long, Decimal128, Binary, Timestamp } = BSON;
const canonical = (value: unknown) => EJSON.stringify(value, { relaxed: false });
const rows = (db: Db, collection: string) => db.collection(collection).find({}, { promoteValues: false }).collation({ locale: "simple" }).sort({ _id: 1 }).toArray();
const restoreCli = async (dir: string, target: Db, force = false) => {
  try {
    const result = await promisify(execFile)(process.execPath, ["--import", "tsx", path.resolve("scripts/restore-db.mts"), dir, "--db", target.databaseName, ...(force ? ["--force"] : [])], {
      cwd: process.cwd(), env: { ...process.env, MONGODB_URI: testUri, MONGODB_DB: target.databaseName }, timeout: 30_000,
    });
    return { code: 0, output: result.stdout + result.stderr };
  } catch (error) {
    const failure = error as { code: number; stdout?: string; stderr?: string };
    return { code: failure.code, output: (failure.stdout ?? "") + (failure.stderr ?? "") };
  }
};
try {
  await client.connect();
  const source = database("source"), restored = database("restored"), existing = database("existing");
  const uid = new ObjectId(), event = new ObjectId(), now = new Date(), future = new Date(Date.now() + 30 * 86_400_000);
  const code = '// Saved Unicode draft: 😀\n'.repeat(12_000) + 'int main(void) { return 0; }';
  const fixtures: Record<string, Document[]> = {
    ["__proto__"]: [{ _id: new ObjectId(), value: "A valid collection name must not disappear from a plain object map." }],
    users: [{ _id: uid, email: "backup-fixture@example.test", passwordHash: "synthetic-scrypt-hash", xp: new Int32(24), createdAt: now, profilePublic: false,
      numericTypes: { double: new Double(7), smallLong: Long.fromNumber(7), largeLong: Long.fromString("9223372036854775806"), decimal: Decimal128.fromString("123.456"), binary: new Binary(Buffer.from([0, 1, 255])), timestamp: new Timestamp({ t: 100, i: 2 }) } }],
    progress: [{ _id: new ObjectId(), userId: uid, exerciseId: "c-memory/arrays", status: "passed", code, attemptsAtPass: new Int32(1), unaided: true }],
    xp_events: [{ _id: new ObjectId(), userId: uid, source: "lesson", ref: "c-memory/arrays", xp: new Int32(5), at: now }],
    achievements: [{ _id: new ObjectId(), userId: uid, id: "first-pass", unlockedAt: now }],
    reviews: [{ _id: new ObjectId(), userId: uid, exerciseId: "c-memory/arrays", dueAt: future, lastAt: now, interval: new Int32(7), count: new Int32(1),
      pendingEvent: { _id: event, userId: uid, exerciseId: "c-memory/arrays", at: now, source: "coding", correct: true, intervalBefore: new Int32(3), intervalAfter: new Int32(7) } }],
    review_events: [{ _id: new ObjectId(), userId: uid, exerciseId: "c-memory/pointers-swap", retryOf: new ObjectId(), source: "retry", at: now, correct: false }],
    tutor_reservations: [{ _id: new ObjectId(), userId: uid.toHexString(), amount: new Int32(100), keys: [`u:${uid}:2026-11-10`, "g:2026-11-10"], state: "settling", targetMicro: new Int32(25), createdAt: now, recoveryAttemptedAt: now }],
    tutor_spend: [{ _id: new ObjectId(), key: "g:2026-11-10", micro: new Int32(100), createdAt: now, reservations: { synthetic: new Int32(100) } }],
    sessions: [{ _id: new ObjectId(), userId: uid, tokenHash: "synthetic-token-hash", expiresAt: future }],
  };
  for (const [collection, documents] of Object.entries(fixtures)) await source.collection(collection).insertMany(documents);
  await source.createCollection("empty");
  await source.collection("users").createIndex({ email: 1 }, { unique: true });
  await source.collection("users").createIndex({ createdAt: 1 }, { name: "dated_partial", partialFilterExpression: { createdAt: { $gte: new Date("2000-01-01") } } });
  await source.collection("progress").createIndex({ userId: 1, exerciseId: 1 }, { unique: true });
  await source.collection("sessions").createIndex({ expiresAt: 1 }, { expireAfterSeconds: 0 });
  await source.collection("review_events").createIndex({ retryOf: 1 }, { unique: true, partialFilterExpression: { retryOf: { $type: "objectId" } } });
  await source.collection("tutor_spend").createIndex({ key: 1 }, { unique: true });
  await source.collection("tutor_reservations").createIndex({ expiresAt: 1 }, { expireAfterSeconds: 0 });

  const { dir } = await dumpDb(source, source.databaseName);
  const manifest = await readManifest(dir);
  assert.equal(manifest.format, 2, "new metadata requires a version old restorers reject before touching their target");
  assert.equal((await lstat(dir)).mode & 0o777, 0o700);
  for (const file of ["manifest.json", ...Object.values(manifest.collections).map((info) => info.file)]) assert.equal((await lstat(path.join(dir, file))).mode & 0o777, 0o600);
  assert(Object.values(manifest.collections).every((info) => info.sha256 && info.documentsSha256));
  await loadInto(restored, dir, manifest);
  for (const collection of [...Object.keys(fixtures), "empty"]) {
    assert.equal(canonical(await rows(restored, collection)), canonical(await rows(source, collection)), `${collection}: canonical BSON content survives`);
    const indexes = async (db: Db) => (await db.collection(collection).indexes()).map(({ v: _v, ns: _ns, ...index }) => { void _v; void _ns; return index; }).sort((a, b) => a.name!.localeCompare(b.name!));
    assert.equal(canonical(await indexes(restored)), canonical(await indexes(source)), `${collection}: index options survive`);
  }
  await assert.rejects(restored.collection("users").insertOne({ email: "backup-fixture@example.test" }), (error: { code?: number }) => error.code === 11000);
  assert.deepEqual((await verifyBackup(client, dir)).problems, []);
  console.log("Backup canonical BSON, Unicode saved code, XP, review outbox, reservation, indexes and permissions roundtrip passed.");

  await existing.collection("users").insertOne({ _id: uid, email: "keep-existing@example.test" });
  const before = canonical(await rows(existing, "users"));
  const refused = await restoreCli(dir, existing);
  assert.equal(refused.code, 3);
  assert.match(refused.output, /already has data/);
  assert.equal(canonical(await rows(existing, "users")), before);

  const tampered = path.join(scratch, "tampered");
  await cp(dir, tampered, { recursive: true });
  const dataFile = path.join(tampered, manifest.collections.users.file);
  const changed = gunzipSync(await readFile(dataFile)).toString("utf8").replace("backup-fixture@example.test", "edited-fixture@example.test");
  await writeFile(dataFile, gzipSync(changed));
  const rejected = await restoreCli(tampered, existing, true);
  assert.equal(rejected.code, 1);
  assert.match(rejected.output, /SHA-256 mismatch/);
  assert.equal(canonical(await rows(existing, "users")), before, "corrupt backup must fail before --force drops existing data");
  const editedManifest = JSON.parse(JSON.stringify(manifest)) as Manifest;
  editedManifest.collections.users.sha256 = createHash("sha256").update(await readFile(dataFile)).digest("hex");
  await writeFile(path.join(tampered, "manifest.json"), JSON.stringify(editedManifest));
  await assert.rejects(validateBackup(tampered, await readManifest(tampered)), /document digest mismatch/, "canonical document digest independently catches equal-count changes");

  const legacy = path.join(scratch, "legacy");
  await cp(dir, legacy, { recursive: true });
  const oldManifest = JSON.parse(JSON.stringify(manifest)) as Manifest;
  oldManifest.format = 1;
  for (const [collection, info] of Object.entries(oldManifest.collections)) {
    delete info.sha256; delete info.documentsSha256; delete info.indexEncoding;
    info.indexes = (await source.collection(collection).indexes()).filter((index) => index.name !== "_id_");
  }
  await writeFile(path.join(legacy, "manifest.json"), JSON.stringify(oldManifest));
  assert.deepEqual((await verifyBackup(client, legacy)).problems, [], "format-1 backups without hashes still restore");
  await writeFile(path.join(legacy, oldManifest.collections.users.file), "not gzip");
  const corruptLegacy = await restoreCli(legacy, existing, true);
  assert.equal(corruptLegacy.code, 1);
  assert.equal(canonical(await rows(existing, "users")), before, "legacy gzip errors also precede destructive restore steps");

  const escaped = structuredClone(manifest);
  escaped.collections.users.file = "../outside.jsonl.gz";
  await assert.rejects(validateBackup(dir, escaped), /Invalid backup collection entry/);
  const linked = path.join(scratch, "linked");
  await cp(dir, linked, { recursive: true });
  await unlink(path.join(linked, manifest.collections.users.file));
  await symlink(path.join(dir, manifest.collections.users.file), path.join(linked, manifest.collections.users.file));
  await assert.rejects(validateBackup(linked, manifest), /regular file/);
  const replaced = await restoreCli(dir, existing, true);
  assert.equal(replaced.code, 0, replaced.output);
  for (const collection of [...Object.keys(fixtures), "empty"]) assert.equal(canonical(await rows(existing, collection)), canonical(await rows(source, collection)), `${collection}: CLI --force restores verified content`);
  console.log("Existing-target refusal, same-count tampering, legacy compatibility, corrupt gzip and artifact path/symlink checks passed.");
} finally {
  for (const db of databases) await db.dropDatabase();
  await client.close();
  if (oldBackupDir === undefined) delete process.env.BACKUP_DIR; else process.env.BACKUP_DIR = oldBackupDir;
  await rm(scratch, { recursive: true, force: true });
}
